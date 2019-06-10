/* CLI stylizing

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

#ifndef CLI_CLI_STYLE_H
#define CLI_CLI_STYLE_H

#include "ui-file.h"

/* A single CLI style option.  */
class cli_style_option
{
public:

  /* Construct a CLI style option with a foreground color.  */
  cli_style_option (ui_file_style::basic_color fg);

  /* Return a ui_file_style corresponding to the settings in this CLI
     style.  */
  ui_file_style style () const;

  /* Call once to register this CLI style with the CLI engine.  */
  void add_setshow_commands (const char *name,
			     enum command_class theclass,
			     const char *prefix_doc,
			     struct cmd_list_element **set_list,
			     void (*do_set) (const char *args, int from_tty),
			     struct cmd_list_element **show_list,
			     void (*do_show) (const char *args, int from_tty));

  /* Return the 'set style NAME' command list, that can be used
     to build a lambda DO_SET to call add_setshow_commands.  */
  struct cmd_list_element *set_list () { return m_set_list; };

  /* Same as SET_LIST but for the show command list.  */
  struct cmd_list_element *show_list () { return m_show_list; };

private:

  /* The foreground.  */
  const char *m_foreground;
  /* The background.  */
  const char *m_background;
  /* The intensity.  */
  const char *m_intensity;

  /* Storage for prefixes needed when registering the commands.  */
  std::string m_show_prefix;
  std::string m_set_prefix;
  /* Storage for command lists needed when registering
     subcommands.  */
  struct cmd_list_element *m_set_list = nullptr;
  struct cmd_list_element *m_show_list = nullptr;

  /* Callback to show the foreground.  */
  static void do_show_foreground (struct ui_file *file, int from_tty,
				  struct cmd_list_element *cmd,
				  const char *value);
  /* Callback to show the background.  */
  static void do_show_background (struct ui_file *file, int from_tty,
				  struct cmd_list_element *cmd,
				  const char *value);
  /* Callback to show the intensity.  */
  static void do_show_intensity (struct ui_file *file, int from_tty,
				 struct cmd_list_element *cmd,
				 const char *value);
};

/* The file name style.  */
extern cli_style_option file_name_style;

/* The function name style.  */
extern cli_style_option function_name_style;

/* The variable name style.  */
extern cli_style_option variable_name_style;

/* The address style.  */
extern cli_style_option address_style;

/* True if source styling is enabled.  */
extern int source_styling;

/* True if styling is enabled.  */
extern int cli_styling;

#endif /* CLI_CLI_STYLE_H */
