/* TUI layout window management.

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
#include "command.h"
#include "symtab.h"
#include "frame.h"
#include "source.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include <ctype.h>
#include <unordered_map>
#include <unordered_set>

#include "tui/tui.h"
#include "tui/tui-command.h"
#include "tui/tui-data.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-stack.h"
#include "tui/tui-regs.h"
#include "tui/tui-win.h"
#include "tui/tui-winsource.h"
#include "tui/tui-disasm.h"
#include "tui/tui-layout.h"
#include "tui/tui-source.h"
#include "gdb_curses.h"

static void extract_display_start_addr (struct gdbarch **, CORE_ADDR *);

/* The layouts.  */
static std::vector<std::unique_ptr<tui_layout_split>> layouts;

/* The layout that is currently applied.  */
static std::unique_ptr<tui_layout_base> applied_layout;

/* The "skeleton" version of the layout that is currently applied.  */
static tui_layout_split *applied_skeleton;

/* The two special "regs" layouts.  Note that these aren't registered
   as commands and so can never be deleted.  */
static tui_layout_split *src_regs_layout;
static tui_layout_split *asm_regs_layout;

/* See tui-data.h.  */
std::vector<tui_win_info *> tui_windows;

/* When applying a layout, this is the list of all windows that were
   in the previous layout.  This is used to re-use windows when
   changing a layout.  */
static std::vector<tui_win_info *> saved_tui_windows;

/* See tui-layout.h.  */

void
tui_apply_current_layout ()
{
  struct gdbarch *gdbarch;
  CORE_ADDR addr;

  extract_display_start_addr (&gdbarch, &addr);

  saved_tui_windows = std::move (tui_windows);
  tui_windows.clear ();

  for (tui_win_info *win_info : saved_tui_windows)
    win_info->make_visible (false);

  applied_layout->apply (0, 0, tui_term_width (), tui_term_height ());

  /* Keep the list of internal windows up-to-date.  */
  for (int win_type = SRC_WIN; (win_type < MAX_MAJOR_WINDOWS); win_type++)
    if (tui_win_list[win_type] != nullptr
	&& !tui_win_list[win_type]->is_visible ())
      tui_win_list[win_type] = nullptr;

  /* This should always be made visible by a layout.  */
  gdb_assert (TUI_CMD_WIN->is_visible ());

  /* Now delete any window that was not re-applied.  */
  tui_win_info *focus = tui_win_with_focus ();
  for (tui_win_info *win_info : saved_tui_windows)
    {
      if (!win_info->is_visible ())
	{
	  if (focus == win_info)
	    tui_set_win_focus_to (tui_windows[0]);
	  delete win_info;
	}
    }

  if (gdbarch == nullptr && TUI_DISASM_WIN != nullptr)
    tui_get_begin_asm_address (&gdbarch, &addr);
  tui_update_source_windows_with_addr (gdbarch, addr);

  saved_tui_windows.clear ();
}

/* See tui-layout.  */

void
tui_adjust_window_height (struct tui_win_info *win, int new_height)
{
  applied_layout->adjust_size (win->name (), new_height);
}

/* Set the current layout to LAYOUT.  */

static void
tui_set_layout (tui_layout_split *layout)
{
  applied_skeleton = layout;
  applied_layout = layout->clone ();
  tui_apply_current_layout ();
}

/* See tui-layout.h.  */

void
tui_add_win_to_layout (enum tui_win_type type)
{
  gdb_assert (type == SRC_WIN || type == DISASSEM_WIN);

  /* If the window already exists, no need to add it.  */
  if (tui_win_list[type] != nullptr)
    return;

  /* If the window we are trying to replace doesn't exist, we're
     done.  */
  enum tui_win_type other = type == SRC_WIN ? DISASSEM_WIN : SRC_WIN;
  if (tui_win_list[other] == nullptr)
    return;

  const char *name = type == SRC_WIN ? SRC_NAME : DISASSEM_NAME;
  applied_layout->replace_window (tui_win_list[other]->name (), name);
  tui_apply_current_layout ();
}

/* Find LAYOUT in the "layouts" global and return its index.  */

static size_t
find_layout (tui_layout_split *layout)
{
  for (size_t i = 0; i < layouts.size (); ++i)
    {
      if (layout == layouts[i].get ())
	return i;
    }
  gdb_assert_not_reached (_("layout not found!?"));
}

/* Function to set the layout. */

static void
tui_apply_layout (struct cmd_list_element *command,
		  const char *args, int from_tty)
{
  tui_layout_split *layout
    = (tui_layout_split *) get_cmd_context (command);

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  tui_set_layout (layout);
}

/* See tui-layout.h.  */

void
tui_next_layout ()
{
  size_t index = find_layout (applied_skeleton);
  ++index;
  if (index == layouts.size ())
    index = 0;
  tui_set_layout (layouts[index].get ());
}

/* Implement the "layout next" command.  */

static void
tui_next_layout_command (const char *arg, int from_tty)
{
  tui_enable ();
  tui_next_layout ();
}

/* See tui-layout.h.  */

void
tui_set_initial_layout ()
{
  tui_set_layout (layouts[0].get ());
}

/* Implement the "layout prev" command.  */

static void
tui_prev_layout_command (const char *arg, int from_tty)
{
  tui_enable ();
  size_t index = find_layout (applied_skeleton);
  if (index == 0)
    index = layouts.size ();
  --index;
  tui_set_layout (layouts[index].get ());
}


/* See tui-layout.h.  */

void
tui_regs_layout ()
{
  /* If there's already a register window, we're done.  */
  if (TUI_DATA_WIN != nullptr)
    return;

  tui_set_layout (TUI_DISASM_WIN != nullptr
		  ? asm_regs_layout
		  : src_regs_layout);
}

/* Implement the "layout regs" command.  */

static void
tui_regs_layout_command (const char *arg, int from_tty)
{
  tui_enable ();
  tui_regs_layout ();
}

/* See tui-layout.h.  */

void
tui_remove_some_windows ()
{
  tui_win_info *focus = tui_win_with_focus ();

  if (strcmp (focus->name (), CMD_NAME) == 0)
    {
      /* Try leaving the source or disassembly window.  If neither
	 exists, just do nothing.  */
      focus = TUI_SRC_WIN;
      if (focus == nullptr)
	focus = TUI_DISASM_WIN;
      if (focus == nullptr)
	return;
    }

  applied_layout->remove_windows (focus->name ());
  tui_apply_current_layout ();
}

static void
extract_display_start_addr (struct gdbarch **gdbarch_p, CORE_ADDR *addr_p)
{
  if (TUI_SRC_WIN != nullptr)
    TUI_SRC_WIN->display_start_addr (gdbarch_p, addr_p);
  else if (TUI_DISASM_WIN != nullptr)
    TUI_DISASM_WIN->display_start_addr (gdbarch_p, addr_p);
  else
    {
      *gdbarch_p = nullptr;
      *addr_p = 0;
    }
}

void
tui_win_info::resize (int height_, int width_,
		      int origin_x_, int origin_y_)
{
  if (width == width_ && height == height_
      && x == origin_x_ && y == origin_y_
      && handle != nullptr)
    return;

  width = width_;
  height = height_;
  x = origin_x_;
  y = origin_y_;

  if (handle != nullptr)
    {
#ifdef HAVE_WRESIZE
      wresize (handle.get (), height, width);
      mvwin (handle.get (), y, x);
      wmove (handle.get (), 0, 0);
#else
      handle.reset (nullptr);
#endif
    }

  if (handle == nullptr)
    make_window ();

  rerender ();
}



/* Helper function to create one of the built-in (non-locator)
   windows.  */

template<enum tui_win_type V, class T>
static tui_win_info *
make_standard_window (const char *)
{
  if (tui_win_list[V] == nullptr)
    tui_win_list[V] = new T ();
  return tui_win_list[V];
}

/* Helper function to wrap tui_locator_win_info_ptr for
   tui_get_window_by_name.  */

static tui_win_info *
get_locator_window (const char *)
{
  return tui_locator_win_info_ptr ();
}

/* A map holding all the known window types, keyed by name.  Note that
   this is heap-allocated and "leaked" at gdb exit.  This avoids
   ordering issues with destroying elements in the map at shutdown.
   In particular, destroying this map can occur after Python has been
   shut down, causing crashes if any window destruction requires
   running Python code.  */

static std::unordered_map<std::string, window_factory> *known_window_types;

/* Helper function that returns a TUI window, given its name.  */

static tui_win_info *
tui_get_window_by_name (const std::string &name)
{
  for (tui_win_info *window : saved_tui_windows)
    if (name == window->name ())
      return window;

  auto iter = known_window_types->find (name);
  if (iter == known_window_types->end ())
    error (_("Unknown window type \"%s\""), name.c_str ());

  tui_win_info *result = iter->second (name.c_str ());
  if (result == nullptr)
    error (_("Could not create window \"%s\""), name.c_str ());
  return result;
}

/* Initialize the known window types.  */

static void
initialize_known_windows ()
{
  known_window_types = new std::unordered_map<std::string, window_factory>;

  known_window_types->emplace (SRC_NAME,
			       make_standard_window<SRC_WIN,
						    tui_source_window>);
  known_window_types->emplace (CMD_NAME,
			       make_standard_window<CMD_WIN, tui_cmd_window>);
  known_window_types->emplace (DATA_NAME,
			       make_standard_window<DATA_WIN,
						    tui_data_window>);
  known_window_types->emplace (DISASSEM_NAME,
			       make_standard_window<DISASSEM_WIN,
						    tui_disasm_window>);
  known_window_types->emplace (STATUS_NAME, get_locator_window);
}

/* See tui-layout.h.  */

void
tui_register_window (const char *name, window_factory &&factory)
{
  std::string name_copy = name;

  if (name_copy == SRC_NAME || name_copy == CMD_NAME || name_copy == DATA_NAME
      || name_copy == DISASSEM_NAME || name_copy == STATUS_NAME)
    error (_("Window type \"%s\" is built-in"), name);

  known_window_types->emplace (std::move (name_copy),
			       std::move (factory));
}

/* See tui-layout.h.  */

std::unique_ptr<tui_layout_base>
tui_layout_window::clone () const
{
  tui_layout_window *result = new tui_layout_window (m_contents.c_str ());
  return std::unique_ptr<tui_layout_base> (result);
}

/* See tui-layout.h.  */

void
tui_layout_window::apply (int x_, int y_, int width_, int height_)
{
  x = x_;
  y = y_;
  width = width_;
  height = height_;
  gdb_assert (m_window != nullptr);
  m_window->resize (height, width, x, y);
  tui_windows.push_back (m_window);
}

/* See tui-layout.h.  */

void
tui_layout_window::get_sizes (bool height, int *min_value, int *max_value)
{
  if (m_window == nullptr)
    m_window = tui_get_window_by_name (m_contents);
  if (height)
    {
      *min_value = m_window->min_height ();
      *max_value = m_window->max_height ();
    }
  else
    {
      *min_value = m_window->min_width ();
      *max_value = m_window->max_width ();
    }
}

/* See tui-layout.h.  */

bool
tui_layout_window::top_boxed_p () const
{
  gdb_assert (m_window != nullptr);
  return m_window->can_box ();
}

/* See tui-layout.h.  */

bool
tui_layout_window::bottom_boxed_p () const
{
  gdb_assert (m_window != nullptr);
  return m_window->can_box ();
}

/* See tui-layout.h.  */

void
tui_layout_window::replace_window (const char *name, const char *new_window)
{
  if (m_contents == name)
    {
      m_contents = new_window;
      if (m_window != nullptr)
	{
	  m_window->make_visible (false);
	  m_window = tui_get_window_by_name (m_contents);
	}
    }
}

/* See tui-layout.h.  */

void
tui_layout_window::specification (ui_file *output, int depth)
{
  fputs_unfiltered (get_name (), output);
}

/* See tui-layout.h.  */

void
tui_layout_split::add_split (std::unique_ptr<tui_layout_split> &&layout,
			     int weight)
{
  split s = {weight, std::move (layout)};
  m_splits.push_back (std::move (s));
}

/* See tui-layout.h.  */

void
tui_layout_split::add_window (const char *name, int weight)
{
  tui_layout_window *result = new tui_layout_window (name);
  split s = {weight, std::unique_ptr<tui_layout_base> (result)};
  m_splits.push_back (std::move (s));
}

/* See tui-layout.h.  */

std::unique_ptr<tui_layout_base>
tui_layout_split::clone () const
{
  tui_layout_split *result = new tui_layout_split (m_vertical);
  for (const split &item : m_splits)
    {
      std::unique_ptr<tui_layout_base> next = item.layout->clone ();
      split s = {item.weight, std::move (next)};
      result->m_splits.push_back (std::move (s));
    }
  return std::unique_ptr<tui_layout_base> (result);
}

/* See tui-layout.h.  */

void
tui_layout_split::get_sizes (bool height, int *min_value, int *max_value)
{
  *min_value = 0;
  *max_value = 0;
  bool first_time = true;
  for (const split &item : m_splits)
    {
      int new_min, new_max;
      item.layout->get_sizes (height, &new_min, &new_max);
      /* For the mismatch case, the first time through we want to set
	 the min and max to the computed values -- the "first_time"
	 check here is just a funny way of doing that.  */
      if (height == m_vertical || first_time)
	{
	  *min_value += new_min;
	  *max_value += new_max;
	}
      else
	{
	  *min_value = std::max (*min_value, new_min);
	  *max_value = std::min (*max_value, new_max);
	}
      first_time = false;
    }
}

/* See tui-layout.h.  */

bool
tui_layout_split::top_boxed_p () const
{
  if (m_splits.empty ())
    return false;
  return m_splits[0].layout->top_boxed_p ();
}

/* See tui-layout.h.  */

bool
tui_layout_split::bottom_boxed_p () const
{
  if (m_splits.empty ())
    return false;
  return m_splits.back ().layout->top_boxed_p ();
}

/* See tui-layout.h.  */

void
tui_layout_split::set_weights_from_heights ()
{
  for (int i = 0; i < m_splits.size (); ++i)
    m_splits[i].weight = m_splits[i].layout->height;
}

/* See tui-layout.h.  */

tui_adjust_result
tui_layout_split::adjust_size (const char *name, int new_height)
{
  /* Look through the children.  If one is a layout holding the named
     window, we're done; or if one actually is the named window,
     update it.  */
  int found_index = -1;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      tui_adjust_result adjusted
	= m_splits[i].layout->adjust_size (name, new_height);
      if (adjusted == HANDLED)
	return HANDLED;
      if (adjusted == FOUND)
	{
	  if (!m_vertical)
	    return FOUND;
	  found_index = i;
	  break;
	}
    }

  if (found_index == -1)
    return NOT_FOUND;
  if (m_splits[found_index].layout->height == new_height)
    return HANDLED;

  set_weights_from_heights ();
  int delta = m_splits[found_index].weight - new_height;
  m_splits[found_index].weight = new_height;

  /* Distribute the "delta" over the next window; but if the next
     window cannot hold it all, keep going until we either find a
     window that does, or until we loop all the way around.  */
  for (int i = 0; delta != 0 && i < m_splits.size () - 1; ++i)
    {
      int index = (found_index + 1 + i) % m_splits.size ();

      int new_min, new_max;
      m_splits[index].layout->get_sizes (m_vertical, &new_min, &new_max);

      if (delta < 0)
	{
	  /* The primary window grew, so we are trying to shrink other
	     windows.  */
	  int available = m_splits[index].weight - new_min;
	  int shrink_by = std::min (available, -delta);
	  m_splits[index].weight -= shrink_by;
	  delta += shrink_by;
	}
      else
	{
	  /* The primary window shrank, so we are trying to grow other
	     windows.  */
	  int available = new_max - m_splits[index].weight;
	  int grow_by = std::min (available, delta);
	  m_splits[index].weight += grow_by;
	  delta -= grow_by;
	}
    }

  if (delta != 0)
    {
      warning (_("Invalid window height specified"));
      /* Effectively undo any modifications made here.  */
      set_weights_from_heights ();
    }
  else
    {
      /* Simply re-apply the updated layout.  */
      apply (x, y, width, height);
    }

  return HANDLED;
}

/* See tui-layout.h.  */

void
tui_layout_split::apply (int x_, int y_, int width_, int height_)
{
  x = x_;
  y = y_;
  width = width_;
  height = height_;

  struct size_info
  {
    int size;
    int min_size;
    int max_size;
    /* True if this window will share a box border with the previous
       window in the list.  */
    bool share_box;
  };

  std::vector<size_info> info (m_splits.size ());

  /* Step 1: Find the min and max size of each sub-layout.
     Fixed-sized layouts are given their desired size, and then the
     remaining space is distributed among the remaining windows
     according to the weights given.  */
  int available_size = m_vertical ? height : width;
  int last_index = -1;
  int total_weight = 0;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      bool cmd_win_already_exists = TUI_CMD_WIN != nullptr;

      /* Always call get_sizes, to ensure that the window is
	 instantiated.  This is a bit gross but less gross than adding
	 special cases for this in other places.  */
      m_splits[i].layout->get_sizes (m_vertical, &info[i].min_size,
				     &info[i].max_size);

      if (!m_applied
	  && cmd_win_already_exists
	  && m_splits[i].layout->get_name () != nullptr
	  && strcmp (m_splits[i].layout->get_name (), "cmd") == 0)
	{
	  /* If this layout has never been applied, then it means the
	     user just changed the layout.  In this situation, it's
	     desirable to keep the size of the command window the
	     same.  Setting the min and max sizes this way ensures
	     that the resizing step, below, does the right thing with
	     this window.  */
	  info[i].min_size = (m_vertical
			      ? TUI_CMD_WIN->height
			      : TUI_CMD_WIN->width);
	  info[i].max_size = info[i].min_size;
	}

      if (info[i].min_size == info[i].max_size)
	available_size -= info[i].min_size;
      else
	{
	  last_index = i;
	  total_weight += m_splits[i].weight;
	}

      /* Two adjacent boxed windows will share a border, making a bit
	 more size available.  */
      if (i > 0
	  && m_splits[i - 1].layout->bottom_boxed_p ()
	  && m_splits[i].layout->top_boxed_p ())
	info[i].share_box = true;
    }

  /* Step 2: Compute the size of each sub-layout.  Fixed-sized items
     are given their fixed size, while others are resized according to
     their weight.  */
  int used_size = 0;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      /* Compute the height and clamp to the allowable range.  */
      info[i].size = available_size * m_splits[i].weight / total_weight;
      if (info[i].size > info[i].max_size)
	info[i].size = info[i].max_size;
      if (info[i].size < info[i].min_size)
	info[i].size = info[i].min_size;
      /* If there is any leftover size, just redistribute it to the
	 last resizeable window, by dropping it from the allocated
	 size.  We could try to be fancier here perhaps, by
	 redistributing this size among all windows, not just the
	 last window.  */
      if (info[i].min_size != info[i].max_size)
	{
	  used_size += info[i].size;
	  if (info[i].share_box)
	    --used_size;
	}
    }

  /* Allocate any leftover size.  */
  if (available_size >= used_size && last_index != -1)
    info[last_index].size += available_size - used_size;

  /* Step 3: Resize.  */
  int size_accum = 0;
  const int maximum = m_vertical ? height : width;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      /* If we fall off the bottom, just make allocations overlap.
	 GIGO.  */
      if (size_accum + info[i].size > maximum)
	size_accum = maximum - info[i].size;
      else if (info[i].share_box)
	--size_accum;
      if (m_vertical)
	m_splits[i].layout->apply (x, y + size_accum, width, info[i].size);
      else
	m_splits[i].layout->apply (x + size_accum, y, info[i].size, height);
      size_accum += info[i].size;
    }

  m_applied = true;
}

/* See tui-layout.h.  */

void
tui_layout_split::remove_windows (const char *name)
{
  for (int i = 0; i < m_splits.size (); ++i)
    {
      const char *this_name = m_splits[i].layout->get_name ();
      if (this_name == nullptr)
	m_splits[i].layout->remove_windows (name);
      else if (strcmp (this_name, name) == 0
	       || strcmp (this_name, CMD_NAME) == 0
	       || strcmp (this_name, STATUS_NAME) == 0)
	{
	  /* Keep.  */
	}
      else
	{
	  m_splits.erase (m_splits.begin () + i);
	  --i;
	}
    }
}

/* See tui-layout.h.  */

void
tui_layout_split::replace_window (const char *name, const char *new_window)
{
  for (auto &item : m_splits)
    item.layout->replace_window (name, new_window);
}

/* See tui-layout.h.  */

void
tui_layout_split::specification (ui_file *output, int depth)
{
  if (depth > 0)
    fputs_unfiltered ("{", output);

  if (!m_vertical)
    fputs_unfiltered ("-horizontal ", output);

  bool first = true;
  for (auto &item : m_splits)
    {
      if (!first)
	fputs_unfiltered (" ", output);
      first = false;
      item.layout->specification (output, depth + 1);
      fprintf_unfiltered (output, " %d", item.weight);
    }

  if (depth > 0)
    fputs_unfiltered ("}", output);
}

/* Destroy the layout associated with SELF.  */

static void
destroy_layout (struct cmd_list_element *self, void *context)
{
  tui_layout_split *layout = (tui_layout_split *) context;
  size_t index = find_layout (layout);
  layouts.erase (layouts.begin () + index);
}

/* List holding the sub-commands of "layout".  */

static struct cmd_list_element *layout_list;

/* Add a "layout" command with name NAME that switches to LAYOUT.  */

static struct cmd_list_element *
add_layout_command (const char *name, tui_layout_split *layout)
{
  struct cmd_list_element *cmd;

  string_file spec;
  layout->specification (&spec, 0);

  gdb::unique_xmalloc_ptr<char> doc
    (xstrprintf (_("Apply the \"%s\" layout.\n\
This layout was created using:\n\
  tui new-layout %s %s"),
		 name, name, spec.c_str ()));

  cmd = add_cmd (name, class_tui, nullptr, doc.get (), &layout_list);
  set_cmd_context (cmd, layout);
  /* There is no API to set this.  */
  cmd->func = tui_apply_layout;
  cmd->destroyer = destroy_layout;
  cmd->doc_allocated = 1;
  doc.release ();
  layouts.emplace_back (layout);

  return cmd;
}

/* Initialize the standard layouts.  */

static void
initialize_layouts ()
{
  tui_layout_split *layout;

  layout = new tui_layout_split ();
  layout->add_window (SRC_NAME, 2);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  add_layout_command (SRC_NAME, layout);

  layout = new tui_layout_split ();
  layout->add_window (DISASSEM_NAME, 2);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  add_layout_command (DISASSEM_NAME, layout);

  layout = new tui_layout_split ();
  layout->add_window (SRC_NAME, 1);
  layout->add_window (DISASSEM_NAME, 1);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  add_layout_command ("split", layout);

  layout = new tui_layout_split ();
  layout->add_window (DATA_NAME, 1);
  layout->add_window (SRC_NAME, 1);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  layouts.emplace_back (layout);
  src_regs_layout = layout;

  layout = new tui_layout_split ();
  layout->add_window (DATA_NAME, 1);
  layout->add_window (DISASSEM_NAME, 1);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  layouts.emplace_back (layout);
  asm_regs_layout = layout;
}



/* A helper function that returns true if NAME is the name of an
   available window.  */

static bool
validate_window_name (const std::string &name)
{
  auto iter = known_window_types->find (name);
  return iter != known_window_types->end ();
}

/* Implementation of the "tui new-layout" command.  */

static void
tui_new_layout_command (const char *spec, int from_tty)
{
  std::string new_name = extract_arg (&spec);
  if (new_name.empty ())
    error (_("No layout name specified"));
  if (new_name[0] == '-')
    error (_("Layout name cannot start with '-'"));

  bool is_vertical = true;
  spec = skip_spaces (spec);
  if (check_for_argument (&spec, "-horizontal"))
    is_vertical = false;

  std::vector<std::unique_ptr<tui_layout_split>> splits;
  splits.emplace_back (new tui_layout_split (is_vertical));
  std::unordered_set<std::string> seen_windows;
  while (true)
    {
      spec = skip_spaces (spec);
      if (spec[0] == '\0')
	break;

      if (spec[0] == '{')
	{
	  is_vertical = true;
	  spec = skip_spaces (spec + 1);
	  if (check_for_argument (&spec, "-horizontal"))
	    is_vertical = false;
	  splits.emplace_back (new tui_layout_split (is_vertical));
	  continue;
	}

      bool is_close = false;
      std::string name;
      if (spec[0] == '}')
	{
	  is_close = true;
	  ++spec;
	  if (splits.size () == 1)
	    error (_("Extra '}' in layout specification"));
	}
      else
	{
	  name = extract_arg (&spec);
	  if (name.empty ())
	    break;
	  if (!validate_window_name (name))
	    error (_("Unknown window \"%s\""), name.c_str ());
	  if (seen_windows.find (name) != seen_windows.end ())
	    error (_("Window \"%s\" seen twice in layout"), name.c_str ());
	}

      ULONGEST weight = get_ulongest (&spec, '}');
      if ((int) weight != weight)
	error (_("Weight out of range: %s"), pulongest (weight));
      if (is_close)
	{
	  std::unique_ptr<tui_layout_split> last_split
	    = std::move (splits.back ());
	  splits.pop_back ();
	  splits.back ()->add_split (std::move (last_split), weight);
	}
      else
	{
	  splits.back ()->add_window (name.c_str (), weight);
	  seen_windows.insert (name);
	}
    }
  if (splits.size () > 1)
    error (_("Missing '}' in layout specification"));
  if (seen_windows.empty ())
    error (_("New layout does not contain any windows"));
  if (seen_windows.find (CMD_NAME) == seen_windows.end ())
    error (_("New layout does not contain the \"" CMD_NAME "\" window"));

  gdb::unique_xmalloc_ptr<char> cmd_name
    = make_unique_xstrdup (new_name.c_str ());
  std::unique_ptr<tui_layout_split> new_layout = std::move (splits.back ());
  struct cmd_list_element *cmd
    = add_layout_command (cmd_name.get (), new_layout.get ());
  cmd->name_allocated = 1;
  cmd_name.release ();
  new_layout.release ();
}

/* Function to initialize gdb commands, for tui window layout
   manipulation.  */

void _initialize_tui_layout ();
void
_initialize_tui_layout ()
{
  add_basic_prefix_cmd ("layout", class_tui, _("\
Change the layout of windows.\n\
Usage: layout prev | next | LAYOUT-NAME"),
			&layout_list, "layout ", 0, &cmdlist);

  add_cmd ("next", class_tui, tui_next_layout_command,
	   _("Apply the next TUI layout."),
	   &layout_list);
  add_cmd ("prev", class_tui, tui_prev_layout_command,
	   _("Apply the previous TUI layout."),
	   &layout_list);
  add_cmd ("regs", class_tui, tui_regs_layout_command,
	   _("Apply the TUI register layout."),
	   &layout_list);

  add_cmd ("new-layout", class_tui, tui_new_layout_command,
	   _("Create a new TUI layout.\n\
Usage: tui new-layout [-horizontal] NAME WINDOW WEIGHT [WINDOW WEIGHT]...\n\
Create a new TUI layout.  The new layout will be named NAME,\n\
and can be accessed using \"layout NAME\".\n\
The windows will be displayed in the specified order.\n\
A WINDOW can also be of the form:\n\
  { [-horizontal] NAME WEIGHT [NAME WEIGHT]... }\n\
This form indicates a sub-frame.\n\
Each WEIGHT is an integer, which holds the relative size\n\
to be allocated to the window."),
	   tui_get_cmd_list ());

  initialize_layouts ();
  initialize_known_windows ();
}
