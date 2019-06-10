/* CLI colorizing

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

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

#include "defs.h"
#include "cli/cli-cmds.h"
#include "cli/cli-style.h"
#include "source-cache.h"
#include "observable.h"

/* True if styling is enabled.  */

#if defined (__MSDOS__) || defined (__CYGWIN__)
int cli_styling = 0;
#else
int cli_styling = 1;
#endif

/* True if source styling is enabled.  Note that this is only
   consulted when cli_styling is true.  */

int source_styling = 1;

/* Name of colors; must correspond to ui_file_style::basic_color.  */
static const char * const cli_colors[] = {
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

/* Names of intensities; must correspond to
   ui_file_style::intensity.  */
static const char * const cli_intensities[] = {
  "normal",
  "bold",
  "dim",
  nullptr
};

/* See cli-style.h.  */

cli_style_option file_name_style (ui_file_style::GREEN);

/* See cli-style.h.  */

cli_style_option function_name_style (ui_file_style::YELLOW);

/* See cli-style.h.  */

cli_style_option variable_name_style (ui_file_style::CYAN);

/* See cli-style.h.  */

cli_style_option address_style (ui_file_style::BLUE);

/* See cli-style.h.  */

cli_style_option::cli_style_option (ui_file_style::basic_color fg)
  : m_foreground (cli_colors[fg - ui_file_style::NONE]),
    m_background (cli_colors[0]),
    m_intensity (cli_intensities[ui_file_style::NORMAL])
{
}

/* Return the color number corresponding to COLOR.  */

static int
color_number (const char *color)
{
  for (int i = 0; i < ARRAY_SIZE (cli_colors); ++i)
    {
      if (color == cli_colors[i])
	return i - 1;
    }
  gdb_assert_not_reached ("color not found");
}

/* See cli-style.h.  */

ui_file_style
cli_style_option::style () const
{
  int fg = color_number (m_foreground);
  int bg = color_number (m_background);
  ui_file_style::intensity intensity = ui_file_style::NORMAL;

  for (int i = 0; i < ARRAY_SIZE (cli_intensities); ++i)
    {
      if (m_intensity == cli_intensities[i])
	{
	  intensity = (ui_file_style::intensity) i;
	  break;
	}
    }

  return ui_file_style (fg, bg, intensity);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_foreground (struct ui_file *file, int from_tty,
				      struct cmd_list_element *cmd,
				      const char *value)
{
  const char *name = (const char *) get_cmd_context (cmd);
  fprintf_filtered (file, _("The \"%s\" foreground color is: %s\n"),
		    name, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_background (struct ui_file *file, int from_tty,
				      struct cmd_list_element *cmd,
				      const char *value)
{
  const char *name = (const char *) get_cmd_context (cmd);
  fprintf_filtered (file, _("The \"%s\" background color is: %s\n"),
		    name, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_intensity (struct ui_file *file, int from_tty,
				     struct cmd_list_element *cmd,
				     const char *value)
{
  const char *name = (const char *) get_cmd_context (cmd);
  fprintf_filtered (file, _("The \"%s\" display intensity is: %s\n"),
		    name, value);
}

/* See cli-style.h.  */

void
cli_style_option::add_setshow_commands (const char *name,
					enum command_class theclass,
					const char *prefix_doc,
					struct cmd_list_element **set_list,
					void (*do_set) (const char *args,
							int from_tty),
					struct cmd_list_element **show_list,
					void (*do_show) (const char *args,
							 int from_tty))
{
  m_set_prefix = std::string ("set style ") + name + " ";
  m_show_prefix = std::string ("show style ") + name + " ";

  add_prefix_cmd (name, no_class, do_set, prefix_doc, &m_set_list,
		  m_set_prefix.c_str (), 0, set_list);
  add_prefix_cmd (name, no_class, do_show, prefix_doc, &m_show_list,
		  m_show_prefix.c_str (), 0, show_list);

  add_setshow_enum_cmd ("foreground", theclass, cli_colors,
			&m_foreground,
			_("Set the foreground color for this property"),
			_("Show the foreground color for this property"),
			nullptr,
			nullptr,
			do_show_foreground,
			&m_set_list, &m_show_list, (void *) name);
  add_setshow_enum_cmd ("background", theclass, cli_colors,
			&m_background,
			_("Set the background color for this property"),
			_("Show the background color for this property"),
			nullptr,
			nullptr,
			do_show_background,
			&m_set_list, &m_show_list, (void *) name);
  add_setshow_enum_cmd ("intensity", theclass, cli_intensities,
			&m_intensity,
			_("Set the display intensity for this property"),
			_("Show the display intensity for this property"),
			nullptr,
			nullptr,
			do_show_intensity,
			&m_set_list, &m_show_list, (void *) name);
}

static cmd_list_element *style_set_list;
static cmd_list_element *style_show_list;

static void
set_style (const char *arg, int from_tty)
{
  printf_unfiltered (_("\"set style\" must be followed "
		       "by an appropriate subcommand.\n"));
  help_list (style_set_list, "set style ", all_commands, gdb_stdout);
}

static void
show_style (const char *arg, int from_tty)
{
  cmd_show_list (style_show_list, from_tty, "");
}

static void
set_style_enabled  (const char *args, int from_tty, struct cmd_list_element *c)
{
  g_source_cache.clear ();
  gdb::observers::source_styling_changed.notify ();
}

static void
show_style_enabled (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (cli_styling)
    fprintf_filtered (file, _("CLI output styling is enabled.\n"));
  else
    fprintf_filtered (file, _("CLI output styling is disabled.\n"));
}

static void
show_style_sources (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (source_styling)
    fprintf_filtered (file, _("Source code styling is enabled.\n"));
  else
    fprintf_filtered (file, _("Source code styling is disabled.\n"));
}

void
_initialize_cli_style ()
{
  add_prefix_cmd ("style", no_class, set_style, _("\
Style-specific settings\n\
Configure various style-related variables, such as colors"),
		  &style_set_list, "set style ", 0, &setlist);
  add_prefix_cmd ("style", no_class, show_style, _("\
Style-specific settings\n\
Configure various style-related variables, such as colors"),
		  &style_show_list, "show style ", 0, &showlist);

  add_setshow_boolean_cmd ("enabled", no_class, &cli_styling, _("\
Set whether CLI styling is enabled."), _("\
Show whether CLI is enabled."), _("\
If enabled, output to the terminal is styled."),
			   set_style_enabled, show_style_enabled,
			   &style_set_list, &style_show_list);

  add_setshow_boolean_cmd ("sources", no_class, &source_styling, _("\
Set whether source code styling is enabled."), _("\
Show whether source code styling is enabled."), _("\
If enabled, source code is styled.\n"
#ifdef HAVE_SOURCE_HIGHLIGHT
"Note that source styling only works if styling in general is enabled,\n\
see \"show style enabled\"."
#else
"Source highlighting is disabled in this installation of gdb, because\n\
it was not linked against GNU Source Highlight."
#endif
			   ), set_style_enabled, show_style_sources,
			   &style_set_list, &style_show_list);

#define STYLE_ADD_SETSHOW_COMMANDS(STYLE, NAME, PREFIX_DOC)	  \
  STYLE.add_setshow_commands (NAME, no_class, PREFIX_DOC,		\
			      &style_set_list,				\
			      [] (const char *args, int from_tty)	\
			      {						\
				help_list				\
				  (STYLE.set_list (),			\
				   "set style " NAME " ",		\
				   all_commands,			\
				   gdb_stdout);				\
			      },					\
			      &style_show_list,				\
			      [] (const char *args, int from_tty)	\
			      {						\
				cmd_show_list				\
				  (STYLE.show_list (),			\
				   from_tty,				\
				   "");					\
			      })

  STYLE_ADD_SETSHOW_COMMANDS (file_name_style, "filename",
			      _("\
Filename display styling\n\
Configure filename colors and display intensity."));

  STYLE_ADD_SETSHOW_COMMANDS (function_name_style, "function",
			      _("\
Function name display styling\n\
Configure function name colors and display intensity"));

  STYLE_ADD_SETSHOW_COMMANDS (variable_name_style, "variable",
			      _("\
Variable name display styling\n\
Configure variable name colors and display intensity"));

  STYLE_ADD_SETSHOW_COMMANDS (address_style, "address",
			      _("\
Address display styling\n\
Configure address colors and display intensity"));
}
