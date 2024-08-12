/* TUI display source/assembly window.

   Copyright (C) 1998-2023 Free Software Foundation, Inc.

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
#include "value.h"
#include "source.h"
#include "objfiles.h"
#include "filenames.h"
#include "safe-ctype.h"

#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-stack.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-winsource.h"
#include "tui/tui-source.h"
#include "tui/tui-disasm.h"
#include "tui/tui-location.h"
#include "gdb_curses.h"

/* Function to display the "main" routine.  */
void
tui_display_main ()
{
  auto adapter = tui_source_windows ();
  if (adapter.begin () != adapter.end ())
    {
      struct gdbarch *gdbarch;
      CORE_ADDR addr;

      tui_get_begin_asm_address (&gdbarch, &addr);
      if (addr != (CORE_ADDR) 0)
	{
	  struct symtab *s;

	  tui_update_source_windows_with_addr (gdbarch, addr);
	  s = find_pc_line_symtab (addr);
	  tui_location.set_location (s);
	}
    }
}

/* See tui-winsource.h.  */

std::string
tui_copy_source_line (const char **ptr, int *length)
{
  const char *lineptr = *ptr;

  /* Init the line with the line number.  */
  std::string result;

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
      if (c == '\0')
	break;

      ++lineptr;
      ++column;

      auto process_tab = [&] ()
	{
	  int max_tab_len = tui_tab_width;

	  --column;
	  for (int j = column % max_tab_len;
	       j < max_tab_len;
	       column++, j++)
	    result.push_back (' ');
	};

      if (c == '\n' || c == '\r' || c == '\0')
	{
	  /* Nothing.  */
	}
      else if (c == '\t')
	process_tab ();
      else if (ISCNTRL (c))
	{
	  result.push_back ('^');
	  result.push_back (c + 0100);
	  ++column;
	}
      else if (c == 0177)
	{
	  result.push_back ('^');
	  result.push_back ('?');
	  ++column;
	}
      else
	result.push_back (c);
    }
  while (c != '\0' && c != '\n' && c != '\r');

  if (c == '\r' && *lineptr == '\n')
    ++lineptr;
  *ptr = lineptr;

  if (length != nullptr)
    *length = column;

  return result;
}

void
tui_source_window_base::style_changed ()
{
  if (tui_active && is_visible ())
    refill ();
}

/* Function to display source in the source window.  This function
   initializes the horizontal scroll to 0.  */
void
tui_source_window_base::update_source_window
  (struct gdbarch *gdbarch,
   const struct symtab_and_line &sal)
{
  m_horizontal_offset = 0;
  update_source_window_as_is (gdbarch, sal);
}


/* Function to display source in the source/asm window.  This function
   shows the source as specified by the horizontal offset.  */
void
tui_source_window_base::update_source_window_as_is
  (struct gdbarch *gdbarch,
   const struct symtab_and_line &sal)
{
  bool ret = set_contents (gdbarch, sal);

  if (!ret)
    erase_source_content ();
  else
    {
      update_breakpoint_info (nullptr, false);
      show_source_content ();
      update_exec_info ();
    }
}


/* Function to ensure that the source and/or disassemly windows
   reflect the input address.  */
void
tui_update_source_windows_with_addr (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  struct symtab_and_line sal {};
  if (addr != 0)
    sal = find_pc_line (addr, 0);

  for (struct tui_source_window_base *win_info : tui_source_windows ())
    win_info->update_source_window (gdbarch, sal);
}

/* Function to ensure that the source and/or disassembly windows
   reflect the symtab and line.  */
void
tui_update_source_windows_with_line (struct symtab_and_line sal)
{
  struct gdbarch *gdbarch = nullptr;
  if (sal.symtab != nullptr)
    {
      find_line_pc (sal.symtab, sal.line, &sal.pc);
      gdbarch = sal.symtab->compunit ()->objfile ()->arch ();
    }

  for (struct tui_source_window_base *win_info : tui_source_windows ())
    win_info->update_source_window (gdbarch, sal);
}

void
tui_source_window_base::do_erase_source_content (const char *str)
{
  int x_pos;
  int half_width = (width - 2) / 2;

  m_content.clear ();
  if (handle != NULL)
    {
      werase (handle.get ());
      check_and_display_highlight_if_needed ();

      if (strlen (str) >= half_width)
	x_pos = 1;
      else
	x_pos = half_width - strlen (str);
      mvwaddstr (handle.get (),
		 (height / 2),
		 x_pos,
		 (char *) str);

      refresh_window ();
    }
}


/* Redraw the complete line of a source or disassembly window.  */
void
tui_source_window_base::show_source_line (int lineno)
{
  struct tui_source_element *line;

  line = &m_content[lineno];
  if (line->is_exec_point)
    tui_set_reverse_mode (m_pad.get (), true);

  wmove (m_pad.get (), lineno, 0);
  tui_puts (line->line.c_str (), m_pad.get ());
  if (line->is_exec_point)
    tui_set_reverse_mode (m_pad.get (), false);
}

/* See tui-winsource.h.  */

void
tui_source_window_base::refresh_window ()
{
  /* tui_win_info::refresh_window would draw the empty background window to
     the screen, potentially creating a flicker.  */
  wnoutrefresh (handle.get ());

  int pad_width = std::max (m_max_length, width);
  int left_margin = 1 + TUI_EXECINFO_SIZE + extra_margin ();
  int view_width = width - left_margin - 1;
  int pad_x = std::min (pad_width - view_width, m_horizontal_offset);
  /* Ensure that an equal number of scrolls will work if the user
     scrolled beyond where we clip.  */
  m_horizontal_offset = pad_x;
  prefresh (m_pad.get (), 0, pad_x, y + 1, x + left_margin,
	    y + m_content.size (), x + left_margin + view_width - 1);
}

void
tui_source_window_base::show_source_content ()
{
  gdb_assert (!m_content.empty ());

  check_and_display_highlight_if_needed ();

  int pad_width = std::max (m_max_length, width);
  if (m_pad == nullptr || pad_width > getmaxx (m_pad.get ())
      || m_content.size () > getmaxy (m_pad.get ()))
    m_pad.reset (newpad (m_content.size (), pad_width));

  werase (m_pad.get ());
  for (int lineno = 0; lineno < m_content.size (); lineno++)
    show_source_line (lineno);

  refresh_window ();
}

tui_source_window_base::tui_source_window_base ()
{
  m_start_line_or_addr.loa = LOA_ADDRESS;
  m_start_line_or_addr.u.addr = 0;

  gdb::observers::styling_changed.attach
    (std::bind (&tui_source_window::style_changed, this),
     m_observable, "tui-winsource");
}

tui_source_window_base::~tui_source_window_base ()
{
  gdb::observers::styling_changed.detach (m_observable);
}

/* See tui-data.h.  */

void
tui_source_window_base::update_tab_width ()
{
  werase (handle.get ());
  rerender ();
}

void
tui_source_window_base::rerender ()
{
  if (!m_content.empty ())
    {
      struct symtab_and_line cursal
	= get_current_source_symtab_and_line ();

      if (m_start_line_or_addr.loa == LOA_LINE)
	cursal.line = m_start_line_or_addr.u.line_no;
      else
	cursal.pc = m_start_line_or_addr.u.addr;
      update_source_window (m_gdbarch, cursal);
    }
  else if (deprecated_safe_get_selected_frame () != NULL)
    {
      struct symtab_and_line cursal
	= get_current_source_symtab_and_line ();
      frame_info_ptr frame = deprecated_safe_get_selected_frame ();
      struct gdbarch *gdbarch = get_frame_arch (frame);

      struct symtab *s = find_pc_line_symtab (get_frame_pc (frame));
      if (this != TUI_SRC_WIN)
	find_line_pc (s, cursal.line, &cursal.pc);
      update_source_window (gdbarch, cursal);
    }
  else
    erase_source_content ();
}

/* See tui-data.h.  */

void
tui_source_window_base::refill ()
{
  symtab_and_line sal {};

  if (this == TUI_SRC_WIN)
    {
      sal = get_current_source_symtab_and_line ();
      if (sal.symtab == NULL)
	{
	  frame_info_ptr fi = deprecated_safe_get_selected_frame ();
	  if (fi != nullptr)
	    sal = find_pc_line (get_frame_pc (fi), 0);
	}
    }

  if (sal.pspace == nullptr)
    sal.pspace = current_program_space;

  if (m_start_line_or_addr.loa == LOA_LINE)
    sal.line = m_start_line_or_addr.u.line_no;
  else
    sal.pc = m_start_line_or_addr.u.addr;

  update_source_window_as_is (m_gdbarch, sal);
}

/* Scroll the source forward or backward horizontally.  */

void
tui_source_window_base::do_scroll_horizontal (int num_to_scroll)
{
  if (!m_content.empty ())
    {
      int offset = m_horizontal_offset + num_to_scroll;
      if (offset < 0)
	offset = 0;
      m_horizontal_offset = offset;
      refresh_window ();
    }
}


/* Set or clear the is_exec_point flag in the line whose line is
   line_no.  */

void
tui_source_window_base::set_is_exec_point_at (struct tui_line_or_address l)
{
  bool changed = false;
  int i;

  i = 0;
  while (i < m_content.size ())
    {
      bool new_state;
      struct tui_line_or_address content_loa =
	m_content[i].line_or_addr;

      if (content_loa.loa == l.loa
	  && ((l.loa == LOA_LINE && content_loa.u.line_no == l.u.line_no)
	      || (l.loa == LOA_ADDRESS && content_loa.u.addr == l.u.addr)))
	new_state = true;
      else
	new_state = false;
      if (new_state != m_content[i].is_exec_point)
	{
	  changed = true;
	  m_content[i].is_exec_point = new_state;
	}
      i++;
    }
  if (changed)
    refill ();
}

/* See tui-winsource.h.  */

void
tui_update_all_breakpoint_info (struct breakpoint *being_deleted)
{
  for (tui_source_window_base *win : tui_source_windows ())
    {
      if (win->update_breakpoint_info (being_deleted, false))
	win->update_exec_info ();
    }
}


/* Scan the source window and the breakpoints to update the break_mode
   information for each line.

   Returns true if something changed and the execution window must be
   refreshed.  */

bool
tui_source_window_base::update_breakpoint_info
  (struct breakpoint *being_deleted, bool current_only)
{
  int i;
  bool need_refresh = false;

  for (i = 0; i < m_content.size (); i++)
    {
      struct tui_source_element *line;

      line = &m_content[i];
      if (current_only && !line->is_exec_point)
	 continue;

      /* Scan each breakpoint to see if the current line has something to
	 do with it.  Identify enable/disabled breakpoints as well as
	 those that we already hit.  */
      tui_bp_flags mode = 0;
      for (breakpoint *bp : all_breakpoints ())
	{
	  if (bp == being_deleted)
	    continue;

	  for (bp_location *loc : bp->locations ())
	    {
	      if (location_matches_p (loc, i))
		{
		  if (bp->enable_state == bp_disabled)
		    mode |= TUI_BP_DISABLED;
		  else
		    mode |= TUI_BP_ENABLED;
		  if (bp->hit_count)
		    mode |= TUI_BP_HIT;
		  if (bp->loc->cond)
		    mode |= TUI_BP_CONDITIONAL;
		  if (bp->type == bp_hardware_breakpoint)
		    mode |= TUI_BP_HARDWARE;
		}
	    }
	}

      if (line->break_mode != mode)
	{
	  line->break_mode = mode;
	  need_refresh = true;
	}
    }
  return need_refresh;
}

/* Function to initialize the content of the execution info window,
   based upon the input window which is either the source or
   disassembly window.  */
void
tui_source_window_base::update_exec_info ()
{
  update_breakpoint_info (nullptr, true);
  for (int i = 0; i < m_content.size (); i++)
    {
      struct tui_source_element *src_element = &m_content[i];
      char element[TUI_EXECINFO_SIZE] = "   ";

      /* Now update the exec info content based upon the state
	 of each line as indicated by the source content.  */
      tui_bp_flags mode = src_element->break_mode;
      if (mode & TUI_BP_HIT)
	element[TUI_BP_HIT_POS] = (mode & TUI_BP_HARDWARE) ? 'H' : 'B';
      else if (mode & (TUI_BP_ENABLED | TUI_BP_DISABLED))
	element[TUI_BP_HIT_POS] = (mode & TUI_BP_HARDWARE) ? 'h' : 'b';

      if (mode & TUI_BP_ENABLED)
	element[TUI_BP_BREAK_POS] = '+';
      else if (mode & TUI_BP_DISABLED)
	element[TUI_BP_BREAK_POS] = '-';

      if (src_element->is_exec_point)
	element[TUI_EXEC_POS] = '>';

      mvwaddstr (handle.get (), i + 1, 1, element);

      show_line_number (i);
    }
  refresh_window ();
}
