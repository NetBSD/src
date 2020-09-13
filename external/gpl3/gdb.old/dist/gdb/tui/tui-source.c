/* TUI display source window.

   Copyright (C) 1998-2019 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "source.h"
#include "objfiles.h"
#include "filenames.h"
#include "source-cache.h"

#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-stack.h"
#include "tui/tui-winsource.h"
#include "tui/tui-source.h"
#include "gdb_curses.h"

/* A helper function for tui_set_source_content that extracts some
   source text from PTR.  LINE_NO is the line number; FIRST_COL is the
   first column to extract, and LINE_WIDTH is the number of characters
   to display.  Returns a string holding the desired text.  */

static std::string
copy_source_line (const char **ptr, int line_no, int first_col,
		  int line_width)
{
  const char *lineptr = *ptr;

  /* Init the line with the line number.  */
  std::string result = string_printf ("%-6d", line_no);
  int len = result.size ();
  len = len - ((len / tui_tab_width) * tui_tab_width);
  result.append (len, ' ');

  int column = 0;
  char c;
  do
    {
      int skip_bytes;

      c = *lineptr;
      if (c == '\033' && skip_ansi_escape (lineptr, &skip_bytes))
	{
	  /* We always have to preserve escapes.  */
	  result.append (lineptr, lineptr + skip_bytes);
	  lineptr += skip_bytes;
	  continue;
	}

      ++lineptr;
      ++column;

      auto process_tab = [&] ()
	{
	  int max_tab_len = tui_tab_width;

	  --column;
	  for (int j = column % max_tab_len;
	       j < max_tab_len && column < first_col + line_width;
	       column++, j++)
	    if (column >= first_col)
	      result.push_back (' ');
	};

      /* We have to process all the text in order to pick up all the
	 escapes.  */
      if (column <= first_col || column > first_col + line_width)
	{
	  if (c == '\t')
	    process_tab ();
	  continue;
	}

      if (c == '\n' || c == '\r' || c == '\0')
	{
	  /* Nothing.  */
	}
      else if (c < 040 && c != '\t')
	{
	  result.push_back ('^');
	  result.push_back (c + 0100);
	}
      else if (c == 0177)
	{
	  result.push_back ('^');
	  result.push_back ('?');
	}
      else if (c == '\t')
	process_tab ();
      else
	result.push_back (c);
    }
  while (c != '\0' && c != '\n' && c != '\r');

  if (c == '\r' && *lineptr == '\n')
    ++lineptr;
  *ptr = lineptr;

  return result;
}

/* Function to display source in the source window.  */
enum tui_status
tui_set_source_content (struct symtab *s, 
			int line_no,
			int noerror)
{
  enum tui_status ret = TUI_FAILURE;

  if (s != (struct symtab *) NULL)
    {
      int line_width, nlines;

      if ((ret = tui_alloc_source_buffer (TUI_SRC_WIN)) == TUI_SUCCESS)
	{
	  line_width = TUI_SRC_WIN->generic.width - 1;
	  /* Take hilite (window border) into account, when
	     calculating the number of lines.  */
	  nlines = (line_no + (TUI_SRC_WIN->generic.height - 2)) - line_no;

	  std::string srclines;
	  if (!g_source_cache.get_source_lines (s, line_no, line_no + nlines,
						&srclines))
	    {
	      if (!noerror)
		{
		  const char *filename = symtab_to_filename_for_display (s);
		  char *name = (char *) alloca (strlen (filename) + 100);

		  sprintf (name, "%s:%d", filename, line_no);
		  print_sys_errmsg (name, errno);
		}
	      ret = TUI_FAILURE;
	    }
	  else
	    {
	      int cur_line_no, cur_line;
	      struct tui_gen_win_info *locator
		= tui_locator_win_info_ptr ();
	      struct tui_source_info *src
		= &TUI_SRC_WIN->detail.source_info;
	      const char *s_filename = symtab_to_filename_for_display (s);

	      if (TUI_SRC_WIN->generic.title)
		xfree (TUI_SRC_WIN->generic.title);
	      TUI_SRC_WIN->generic.title = xstrdup (s_filename);

	      xfree (src->fullname);
	      src->fullname = xstrdup (symtab_to_fullname (s));

	      cur_line = 0;
	      src->gdbarch = get_objfile_arch (SYMTAB_OBJFILE (s));
	      src->start_line_or_addr.loa = LOA_LINE;
	      cur_line_no = src->start_line_or_addr.u.line_no = line_no;

	      const char *iter = srclines.c_str ();
	      while (cur_line < nlines)
		{
		  struct tui_win_element *element
		    = TUI_SRC_WIN->generic.content[cur_line];

		  std::string text;
		  if (*iter != '\0')
		    text = copy_source_line (&iter, cur_line_no,
					     src->horizontal_offset,
					     line_width);

		  /* Set whether element is the execution point
		     and whether there is a break point on it.  */
		  element->which_element.source.line_or_addr.loa =
		    LOA_LINE;
		  element->which_element.source.line_or_addr.u.line_no =
		    cur_line_no;
		  element->which_element.source.is_exec_point =
		    (filename_cmp (locator->content[0]
				   ->which_element.locator.full_name,
				   symtab_to_fullname (s)) == 0
		     && cur_line_no
		     == locator->content[0]
		     ->which_element.locator.line_no);

		  xfree (TUI_SRC_WIN->generic.content[cur_line]
			 ->which_element.source.line);
		  TUI_SRC_WIN->generic.content[cur_line]
		    ->which_element.source.line
		    = xstrdup (text.c_str ());

		  cur_line++;
		  cur_line_no++;
		}
	      TUI_SRC_WIN->generic.content_size = nlines;
	      ret = TUI_SUCCESS;
	    }
	}
    }
  return ret;
}


/* elz: This function sets the contents of the source window to empty
   except for a line in the middle with a warning message about the
   source not being available.  This function is called by
   tui_erase_source_contents(), which in turn is invoked when the
   source files cannot be accessed.  */

void
tui_set_source_content_nil (struct tui_win_info *win_info, 
			    const char *warning_string)
{
  int line_width;
  int n_lines;
  int curr_line = 0;

  line_width = win_info->generic.width - 1;
  n_lines = win_info->generic.height - 2;

  /* Set to empty each line in the window, except for the one which
     contains the message.  */
  while (curr_line < win_info->generic.content_size)
    {
      /* Set the information related to each displayed line to null:
         i.e. the line number is 0, there is no bp, it is not where
         the program is stopped.  */

      struct tui_win_element *element = win_info->generic.content[curr_line];

      element->which_element.source.line_or_addr.loa = LOA_LINE;
      element->which_element.source.line_or_addr.u.line_no = 0;
      element->which_element.source.is_exec_point = FALSE;
      element->which_element.source.has_break = FALSE;

      /* Set the contents of the line to blank.  */
      element->which_element.source.line[0] = (char) 0;

      /* If the current line is in the middle of the screen, then we
         want to display the 'no source available' message in it.
         Note: the 'weird' arithmetic with the line width and height
         comes from the function tui_erase_source_content().  We need
         to keep the screen and the window's actual contents in
         synch.  */

      if (curr_line == (n_lines / 2 + 1))
	{
	  int xpos;
	  int warning_length = strlen (warning_string);
	  char *src_line;

	  if (warning_length >= ((line_width - 1) / 2))
	    xpos = 1;
	  else
	    xpos = (line_width - 1) / 2 - warning_length;

	  src_line = xstrprintf ("%s%s", n_spaces (xpos), warning_string);
	  xfree (element->which_element.source.line);
	  element->which_element.source.line = src_line;
	}

      curr_line++;
    }
}


/* Function to display source in the source window.  This function
   initializes the horizontal scroll to 0.  */
void
tui_show_symtab_source (struct gdbarch *gdbarch, struct symtab *s,
			struct tui_line_or_address line, 
			int noerror)
{
  TUI_SRC_WIN->detail.source_info.horizontal_offset = 0;
  tui_update_source_window_as_is (TUI_SRC_WIN, gdbarch, s, line, noerror);
}


/* Answer whether the source is currently displayed in the source
   window.  */
int
tui_source_is_displayed (const char *fullname)
{
  return (TUI_SRC_WIN != NULL
	  && TUI_SRC_WIN->generic.content_in_use 
	  && (filename_cmp (tui_locator_win_info_ptr ()->content[0]
			      ->which_element.locator.full_name,
			    fullname) == 0));
}


/* Scroll the source forward or backward vertically.  */
void
tui_vertical_source_scroll (enum tui_scroll_direction scroll_direction,
			    int num_to_scroll)
{
  if (TUI_SRC_WIN->generic.content != NULL)
    {
      struct tui_line_or_address l;
      struct symtab *s;
      tui_win_content content = TUI_SRC_WIN->generic.content;
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();

      if (cursal.symtab == (struct symtab *) NULL)
	s = find_pc_line_symtab (get_frame_pc (get_selected_frame (NULL)));
      else
	s = cursal.symtab;

      l.loa = LOA_LINE;
      if (scroll_direction == FORWARD_SCROLL)
	{
	  l.u.line_no = content[0]->which_element.source.line_or_addr.u.line_no
	    + num_to_scroll;
	  if (l.u.line_no > s->nlines)
	    /* line = s->nlines - win_info->generic.content_size + 1; */
	    /* elz: fix for dts 23398.  */
	    l.u.line_no
	      = content[0]->which_element.source.line_or_addr.u.line_no;
	}
      else
	{
	  l.u.line_no = content[0]->which_element.source.line_or_addr.u.line_no
	    - num_to_scroll;
	  if (l.u.line_no <= 0)
	    l.u.line_no = 1;
	}

      print_source_lines (s, l.u.line_no, l.u.line_no + 1, 0);
    }
}
