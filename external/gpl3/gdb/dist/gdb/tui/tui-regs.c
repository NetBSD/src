/* TUI display registers in window.

   Copyright (C) 1998-2020 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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
#include "arch-utils.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "frame.h"
#include "regcache.h"
#include "inferior.h"
#include "target.h"
#include "tui/tui-layout.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-file.h"
#include "tui/tui-regs.h"
#include "tui/tui-io.h"
#include "reggroups.h"
#include "valprint.h"
#include "completer.h"

#include "gdb_curses.h"

/* A subclass of string_file that expands tab characters.  */
class tab_expansion_file : public string_file
{
public:

  tab_expansion_file () = default;

  void write (const char *buf, long length_buf) override;

private:

  int m_column = 0;
};

void
tab_expansion_file::write (const char *buf, long length_buf)
{
  for (long i = 0; i < length_buf; ++i)
    {
      if (buf[i] == '\t')
	{
	  do
	    {
	      string_file::write (" ", 1);
	      ++m_column;
	    }
	  while ((m_column % 8) != 0);
	}
      else
	{
	  string_file::write (&buf[i], 1);
	  if (buf[i] == '\n')
	    m_column = 0;
	  else
	    ++m_column;
	}
    }
}

/* Get the register from the frame and return a printable
   representation of it.  */

static std::string
tui_register_format (struct frame_info *frame, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  /* Expand tabs into spaces, since ncurses on MS-Windows doesn't.  */
  tab_expansion_file stream;

  scoped_restore save_pagination
    = make_scoped_restore (&pagination_enabled, 0);
  scoped_restore save_stdout
    = make_scoped_restore (&gdb_stdout, &stream);

  gdbarch_print_registers_info (gdbarch, &stream, frame, regnum, 1);

  /* Remove the possible \n.  */
  std::string &str = stream.string ();
  if (!str.empty () && str.back () == '\n')
    str.resize (str.size () - 1);

  return str;
}

/* Get the register value from the given frame and format it for the
   display.  When changep is set, check if the new register value has
   changed with respect to the previous call.  */
static void
tui_get_register (struct frame_info *frame,
                  struct tui_data_item_window *data, 
		  int regnum, bool *changedp)
{
  if (changedp)
    *changedp = false;
  if (target_has_registers)
    {
      std::string new_content = tui_register_format (frame, regnum);

      if (changedp != NULL && data->content != new_content)
	*changedp = true;

      data->content = std::move (new_content);
    }
}

/* See tui-regs.h.  */

int
tui_data_window::last_regs_line_no () const
{
  int num_lines = m_regs_content.size () / m_regs_column_count;
  if (m_regs_content.size () % m_regs_column_count)
    num_lines++;
  return num_lines;
}

/* See tui-regs.h.  */

int
tui_data_window::line_from_reg_element_no (int element_no) const
{
  if (element_no < m_regs_content.size ())
    {
      int i, line = (-1);

      i = 1;
      while (line == (-1))
	{
	  if (element_no < m_regs_column_count * i)
	    line = i - 1;
	  else
	    i++;
	}

      return line;
    }
  else
    return (-1);
}

/* See tui-regs.h.  */

int
tui_data_window::first_reg_element_no_inline (int line_no) const
{
  if (line_no * m_regs_column_count <= m_regs_content.size ())
    return ((line_no + 1) * m_regs_column_count) - m_regs_column_count;
  else
    return (-1);
}

/* Show the registers of the given group in the data window
   and refresh the window.  */
void
tui_data_window::show_registers (struct reggroup *group)
{
  if (group == 0)
    group = general_reggroup;

  if (target_has_registers && target_has_stack && target_has_memory)
    {
      show_register_group (group, get_selected_frame (NULL),
			   group == m_current_group);

      /* Clear all notation of changed values.  */
      for (auto &&data_item_win : m_regs_content)
	data_item_win.highlight = false;
      m_current_group = group;
    }
  else
    {
      m_current_group = 0;
      m_regs_content.clear ();
    }

  rerender ();
}


/* Set the data window to display the registers of the register group
   using the given frame.  Values are refreshed only when
   refresh_values_only is true.  */

void
tui_data_window::show_register_group (struct reggroup *group,
				      struct frame_info *frame, 
				      bool refresh_values_only)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int nr_regs;
  int regnum, pos;

  /* Make a new title showing which group we display.  */
  title = string_printf ("Register group: %s", reggroup_name (group));

  /* See how many registers must be displayed.  */
  nr_regs = 0;
  for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      const char *name;

      /* Must be in the group.  */
      if (!gdbarch_register_reggroup_p (gdbarch, regnum, group))
	continue;

      /* If the register name is empty, it is undefined for this
	 processor, so don't display anything.  */
      name = gdbarch_register_name (gdbarch, regnum);
      if (name == 0 || *name == '\0')
	continue;

      nr_regs++;
    }

  m_regs_content.resize (nr_regs);

  /* Now set the register names and values.  */
  pos = 0;
  for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      struct tui_data_item_window *data_item_win;
      const char *name;

      /* Must be in the group.  */
      if (!gdbarch_register_reggroup_p (gdbarch, regnum, group))
	continue;

      /* If the register name is empty, it is undefined for this
	 processor, so don't display anything.  */
      name = gdbarch_register_name (gdbarch, regnum);
      if (name == 0 || *name == '\0')
	continue;

      data_item_win = &m_regs_content[pos];
      if (!refresh_values_only)
	{
	  data_item_win->regno = regnum;
	  data_item_win->highlight = false;
	}
      tui_get_register (frame, data_item_win, regnum, 0);
      pos++;
    }
}

/* See tui-regs.h.  */

void
tui_data_window::display_registers_from (int start_element_no)
{
  int max_len = 0;
  for (auto &&data_item_win : m_regs_content)
    {
      int len = data_item_win.content.size ();

      if (len > max_len)
	max_len = len;
    }
  m_item_width = max_len + 1;
  int i = start_element_no;

  m_regs_column_count = (width - 2) / m_item_width;
  if (m_regs_column_count == 0)
    m_regs_column_count = 1;
  m_item_width = (width - 2) / m_regs_column_count;

  /* Now create each data "sub" window, and write the display into
     it.  */
  int cur_y = 1;
  while (i < m_regs_content.size () && cur_y <= height - 2)
    {
      for (int j = 0;
	   j < m_regs_column_count && i < m_regs_content.size ();
	   j++)
	{
	  /* Create the window if necessary.  */
	  m_regs_content[i].x = (m_item_width * j) + 1;
	  m_regs_content[i].y = cur_y;
	  m_regs_content[i].visible = true;
	  m_regs_content[i].rerender (handle.get (), m_item_width);
	  i++;		/* Next register.  */
	}
      cur_y++;		/* Next row.  */
    }
}

/* See tui-regs.h.  */

void
tui_data_window::display_reg_element_at_line (int start_element_no,
					      int start_line_no)
{
  int element_no = start_element_no;

  if (start_element_no != 0 && start_line_no != 0)
    {
      int last_line_no, first_line_on_last_page;

      last_line_no = last_regs_line_no ();
      first_line_on_last_page = last_line_no - (height - 2);
      if (first_line_on_last_page < 0)
	first_line_on_last_page = 0;

      /* If the element_no causes us to scroll past the end of the
	 registers, adjust what element to really start the
	 display at.  */
      if (start_line_no > first_line_on_last_page)
	element_no = first_reg_element_no_inline (first_line_on_last_page);
    }
  display_registers_from (element_no);
}

/* See tui-regs.h.  */

int
tui_data_window::display_registers_from_line (int line_no)
{
  int element_no;

  if (line_no < 0)
    line_no = 0;
  else
    {
      /* Make sure that we don't display off the end of the
	 registers.  */
      if (line_no >= last_regs_line_no ())
	{
	  line_no = line_from_reg_element_no (m_regs_content.size () - 1);
	  if (line_no < 0)
	    line_no = 0;
	}
    }

  element_no = first_reg_element_no_inline (line_no);
  if (element_no < m_regs_content.size ())
    display_reg_element_at_line (element_no, line_no);
  else
    line_no = (-1);

  return line_no;
}


/* Answer the index first element displayed.  If none are displayed,
   then return (-1).  */
int
tui_data_window::first_data_item_displayed ()
{
  for (int i = 0; i < m_regs_content.size (); i++)
    {
      if (m_regs_content[i].visible)
	return i;
    }

  return -1;
}

/* See tui-regs.h.  */

void
tui_data_window::delete_data_content_windows ()
{
  for (auto &win : m_regs_content)
    win.visible = false;
}


void
tui_data_window::erase_data_content (const char *prompt)
{
  werase (handle.get ());
  check_and_display_highlight_if_needed ();
  if (prompt != NULL)
    {
      int half_width = (width - 2) / 2;
      int x_pos;

      if (strlen (prompt) >= half_width)
	x_pos = 1;
      else
	x_pos = half_width - strlen (prompt);
      mvwaddstr (handle.get (), (height / 2), x_pos, (char *) prompt);
    }
  tui_wrefresh (handle.get ());
}

/* See tui-regs.h.  */

void
tui_data_window::rerender ()
{
  if (m_regs_content.empty ())
    erase_data_content (_("[ Register Values Unavailable ]"));
  else
    {
      erase_data_content (NULL);
      delete_data_content_windows ();
      display_registers_from (0);
    }
}


/* Scroll the data window vertically forward or backward.  */
void
tui_data_window::do_scroll_vertical (int num_to_scroll)
{
  int first_element_no;
  int first_line = (-1);

  first_element_no = first_data_item_displayed ();
  if (first_element_no < m_regs_content.size ())
    first_line = line_from_reg_element_no (first_element_no);
  else
    { /* Calculate the first line from the element number which is in
        the general data content.  */
    }

  if (first_line >= 0)
    {
      first_line += num_to_scroll;
      erase_data_content (NULL);
      delete_data_content_windows ();
      display_registers_from_line (first_line);
    }
}

/* This function check all displayed registers for changes in values,
   given a particular frame.  If the values have changed, they are
   updated with the new value and highlighted.  */
void
tui_data_window::check_register_values (struct frame_info *frame)
{
  if (m_regs_content.empty ())
    show_registers (m_current_group);
  else
    {
      for (auto &&data_item_win : m_regs_content)
	{
	  int was_hilighted;

	  was_hilighted = data_item_win.highlight;

	  tui_get_register (frame, &data_item_win,
			    data_item_win.regno,
			    &data_item_win.highlight);

	  if (data_item_win.highlight || was_hilighted)
	    data_item_win.rerender (handle.get (), m_item_width);
	}
    }

  tui_wrefresh (handle.get ());
}

/* Display a register in a window.  If hilite is TRUE, then the value
   will be displayed in reverse video.  */
void
tui_data_item_window::rerender (WINDOW *handle, int field_width)
{
  if (highlight)
    /* We ignore the return value, casting it to void in order to avoid
       a compiler warning.  The warning itself was introduced by a patch
       to ncurses 5.7 dated 2009-08-29, changing this macro to expand
       to code that causes the compiler to generate an unused-value
       warning.  */
    (void) wstandout (handle);
      
  mvwaddnstr (handle, y, x, content.c_str (), field_width - 1);
  waddstr (handle, n_spaces (field_width - content.size ()));

  if (highlight)
    /* We ignore the return value, casting it to void in order to avoid
       a compiler warning.  The warning itself was introduced by a patch
       to ncurses 5.7 dated 2009-08-29, changing this macro to expand
       to code that causes the compiler to generate an unused-value
       warning.  */
    (void) wstandend (handle);
}

/* Helper for "tui reg next", wraps a call to REGGROUP_NEXT, but adds wrap
   around behaviour.  Returns the next register group, or NULL if the
   register window is not currently being displayed.  */

static struct reggroup *
tui_reg_next (struct reggroup *current_group, struct gdbarch *gdbarch)
{
  struct reggroup *group = NULL;

  if (current_group != NULL)
    {
      group = reggroup_next (gdbarch, current_group);
      if (group == NULL)
        group = reggroup_next (gdbarch, NULL);
    }
  return group;
}

/* Helper for "tui reg prev", wraps a call to REGGROUP_PREV, but adds wrap
   around behaviour.  Returns the previous register group, or NULL if the
   register window is not currently being displayed.  */

static struct reggroup *
tui_reg_prev (struct reggroup *current_group, struct gdbarch *gdbarch)
{
  struct reggroup *group = NULL;

  if (current_group != NULL)
    {
      group = reggroup_prev (gdbarch, current_group);
      if (group == NULL)
	group = reggroup_prev (gdbarch, NULL);
    }
  return group;
}

/* Implement the 'tui reg' command.  Changes the register group displayed
   in the tui register window.  Displays the tui register window if it is
   not already on display.  */

static void
tui_reg_command (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (args != NULL)
    {
      struct reggroup *group, *match = NULL;
      size_t len = strlen (args);

      /* Make sure the curses mode is enabled.  */
      tui_enable ();

      tui_suppress_output suppress;

      /* Make sure the register window is visible.  If not, select an
	 appropriate layout.  We need to do this before trying to run the
	 'next' or 'prev' commands.  */
      if (TUI_DATA_WIN == NULL || !TUI_DATA_WIN->is_visible ())
	tui_regs_layout ();

      struct reggroup *current_group = TUI_DATA_WIN->get_current_group ();
      if (strncmp (args, "next", len) == 0)
	match = tui_reg_next (current_group, gdbarch);
      else if (strncmp (args, "prev", len) == 0)
	match = tui_reg_prev (current_group, gdbarch);

      /* This loop matches on the initial part of a register group
	 name.  If this initial part in ARGS matches only one register
	 group then the switch is made.  */
      for (group = reggroup_next (gdbarch, NULL);
	   group != NULL;
	   group = reggroup_next (gdbarch, group))
	{
	  if (strncmp (reggroup_name (group), args, len) == 0)
	    {
	      if (match != NULL)
		error (_("ambiguous register group name '%s'"), args);
	      match = group;
	    }
	}

      if (match == NULL)
	error (_("unknown register group '%s'"), args);

      TUI_DATA_WIN->show_registers (match);
    }
  else
    {
      struct reggroup *group;
      int first;

      printf_unfiltered (_("\"tui reg\" must be followed by the name of "
			   "either a register group,\nor one of 'next' "
			   "or 'prev'.  Known register groups are:\n"));

      for (first = 1, group = reggroup_next (gdbarch, NULL);
	   group != NULL;
	   first = 0, group = reggroup_next (gdbarch, group))
	{
	  if (!first)
	    printf_unfiltered (", ");
	  printf_unfiltered ("%s", reggroup_name (group));
	}

      printf_unfiltered ("\n");
    }
}

/* Complete names of register groups, and add the special "prev" and "next"
   names.  */

static void
tui_reggroup_completer (struct cmd_list_element *ignore,
			completion_tracker &tracker,
			const char *text, const char *word)
{
  static const char * const extra[] = { "next", "prev", NULL };

  reggroup_completer (ignore, tracker, text, word);

  complete_on_enum (tracker, extra, text, word);
}

void _initialize_tui_regs ();
void
_initialize_tui_regs ()
{
  struct cmd_list_element **tuicmd, *cmd;

  tuicmd = tui_get_cmd_list ();

  cmd = add_cmd ("reg", class_tui, tui_reg_command, _("\
TUI command to control the register window.\n\
Usage: tui reg NAME\n\
NAME is the name of the register group to display"), tuicmd);
  set_cmd_completer (cmd, tui_reggroup_completer);
}
