/* TUI data manipulation routines.

   Copyright (C) 1998-2017 Free Software Foundation, Inc.

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

#ifndef TUI_DATA_H
#define TUI_DATA_H

#include "tui/tui.h"	/* For enum tui_win_type.  */
#include "gdb_curses.h"	/* For WINDOW.  */

/* This is a point definition.  */
struct tui_point
{
  int x, y;
};

struct tui_win_element;

/* This describes the content of the window.  */
typedef struct tui_win_element **tui_win_content;

/* Generic window information.  */
struct tui_gen_win_info
{
  WINDOW *handle;	    /* Window handle.  */
  enum tui_win_type type;   /* Type of window.  */
  int width;		    /* Window width.  */
  int height;		    /* Window height.  */
  struct tui_point origin;  /* Origin of window.  */
  tui_win_content content;  /* Content of window.  */
  int content_size;	    /* Size of content (# of elements).  */
  int content_in_use;	    /* Can it be used, or is it already used?  */
  int viewport_height;	    /* Viewport height.  */
  int last_visible_line;    /* Index of last visible line.  */
  int is_visible;	    /* Whether the window is visible or not.  */
  char *title;              /* Window title to display.  */
};

/* Constant definitions.  */
#define DEFAULT_TAB_LEN         8
#define NO_SRC_STRING           "[ No Source Available ]"
#define NO_DISASSEM_STRING      "[ No Assembly Available ]"
#define NO_REGS_STRING          "[ Register Values Unavailable ]"
#define NO_DATA_STRING          "[ No Data Values Displayed ]"
#define MAX_CONTENT_COUNT       100
#define SRC_NAME                "src"
#define CMD_NAME                "cmd"
#define DATA_NAME               "regs"
#define DISASSEM_NAME           "asm"
#define TUI_NULL_STR            ""
#define DEFAULT_HISTORY_COUNT	25
#define BOX_WINDOW              TRUE
#define DONT_BOX_WINDOW         FALSE
#define HILITE                  TRUE
#define NO_HILITE               FALSE
#define WITH_LOCATOR            TRUE
#define NO_LOCATOR              FALSE
#define EMPTY_SOURCE_PROMPT     TRUE
#define NO_EMPTY_SOURCE_PROMPT  FALSE
#define UNDEFINED_ITEM          -1
#define MIN_WIN_HEIGHT          3
#define MIN_CMD_WIN_HEIGHT      3

/* Strings to display in the TUI status line.  */
#define PROC_PREFIX             "In: "
#define LINE_PREFIX             "L"
#define PC_PREFIX               "PC: "
#define SINGLE_KEY              "(SingleKey)"

/* Minimum/Maximum length of some fields displayed in the TUI status
   line.  */
#define MIN_LINE_WIDTH     4	/* Use at least 4 digits for line
				   numbers.  */
#define MIN_PROC_WIDTH    12
#define MAX_TARGET_WIDTH  10
#define MAX_PID_WIDTH     19

/* Scroll direction enum.  */
enum tui_scroll_direction
{
  FORWARD_SCROLL,
  BACKWARD_SCROLL,
  LEFT_SCROLL,
  RIGHT_SCROLL
};


/* General list struct.  */
struct tui_list
{
  struct tui_win_info **list;
  int count;
};


/* The kinds of layouts available.  */
enum tui_layout_type
{
  SRC_COMMAND,
  DISASSEM_COMMAND,
  SRC_DISASSEM_COMMAND,
  SRC_DATA_COMMAND,
  DISASSEM_DATA_COMMAND,
  UNDEFINED_LAYOUT
};

/* Basic data types that can be displayed in the data window.  */
enum tui_data_type
{
  TUI_REGISTER,
  TUI_SCALAR,
  TUI_COMPLEX,
  TUI_STRUCT
};

enum tui_line_or_address_kind
{
  LOA_LINE,
  LOA_ADDRESS
};

/* Structure describing source line or line address.  */
struct tui_line_or_address
{
  enum tui_line_or_address_kind loa;
  union
    {
      int line_no;
      CORE_ADDR addr;
    } u;
};

/* Current Layout definition.  */
struct tui_layout_def
{
  enum tui_win_type display_mode;
  int split;
};

/* Elements in the Source/Disassembly Window.  */
struct tui_source_element
{
  char *line;
  struct tui_line_or_address line_or_addr;
  int is_exec_point;
  int has_break;
};


/* Elements in the data display window content.  */
struct tui_data_element
{
  const char *name;
  int item_no;		/* The register number, or data display
			   number.  */
  enum tui_data_type type;
  void *value;
  int highlight;
  char *content;
};


/* Elements in the command window content.  */
struct tui_command_element
{
  char *line;
};

#ifdef PATH_MAX
# define MAX_LOCATOR_ELEMENT_LEN        PATH_MAX
#else
# define MAX_LOCATOR_ELEMENT_LEN        1024
#endif

/* Elements in the locator window content.  */
struct tui_locator_element
{
  /* Resolved absolute filename as returned by symtab_to_fullname.  */
  char full_name[MAX_LOCATOR_ELEMENT_LEN];
  char proc_name[MAX_LOCATOR_ELEMENT_LEN];
  int line_no;
  CORE_ADDR addr;
  /* Architecture associated with code at this location.  */
  struct gdbarch *gdbarch;
};

/* Flags to tell what kind of breakpoint is at current line.  */
#define TUI_BP_ENABLED      0x01
#define TUI_BP_DISABLED     0x02
#define TUI_BP_HIT          0x04
#define TUI_BP_CONDITIONAL  0x08
#define TUI_BP_HARDWARE     0x10

/* Position of breakpoint markers in the exec info string.  */
#define TUI_BP_HIT_POS      0
#define TUI_BP_BREAK_POS    1
#define TUI_EXEC_POS        2
#define TUI_EXECINFO_SIZE   4

typedef char tui_exec_info_content[TUI_EXECINFO_SIZE];

/* An content element in a window.  */
union tui_which_element
{
  struct tui_source_element source;	/* The source elements.  */
  struct tui_gen_win_info data_window;	/* Data display elements.  */
  struct tui_data_element data;		/* Elements of data_window.  */
  struct tui_command_element command;	/* Command elements.  */
  struct tui_locator_element locator;	/* Locator elements.  */
  tui_exec_info_content simple_string;	/* Simple char based elements.  */
};

struct tui_win_element
{
  int highlight;
  union tui_which_element which_element;
};

/* This struct defines the specific information about a data display
   window.  */
struct tui_data_info
{
  tui_win_content data_content;	/* Start of data display content.  */
  int data_content_count;
  tui_win_content regs_content;	/* Start of regs display content.  */
  int regs_content_count;
  int regs_column_count;
  int display_regs;		/* Should regs be displayed at all?  */
  struct reggroup *current_group;
};


struct tui_source_info
{
  int has_locator;		/* Does locator belongs to this window?  */
  /* Execution information window.  */
  struct tui_gen_win_info *execution_info;
  int horizontal_offset;	/* Used for horizontal scroll.  */
  struct tui_line_or_address start_line_or_addr;

  /* It is the resolved form as returned by symtab_to_fullname.  */
  char *fullname;

  /* Architecture associated with code at this location.  */
  struct gdbarch *gdbarch;
};


struct tui_command_info
{
  int start_line;
};


/* This defines information about each logical window.  */
struct tui_win_info
{
  struct tui_gen_win_info generic;	/* General window information.  */
  union
  {
    struct tui_source_info source_info;
    struct tui_data_info data_display_info;
    struct tui_command_info command_info;
    void *opaque;
  }
  detail;
  int can_highlight;	/* Can this window ever be highlighted?  */
  int is_highlighted;	/* Is this window highlighted?  */
};

extern int tui_win_is_source_type (enum tui_win_type win_type);
extern int tui_win_is_auxillary (enum tui_win_type win_type);
extern int tui_win_has_locator (struct tui_win_info *win_info);
extern void tui_set_win_highlight (struct tui_win_info *win_info,
				   int highlight);


/* Global Data.  */
extern struct tui_win_info *(tui_win_list[MAX_MAJOR_WINDOWS]);

#define TUI_SRC_WIN     tui_win_list[SRC_WIN]
#define TUI_DISASM_WIN	tui_win_list[DISASSEM_WIN]
#define TUI_DATA_WIN    tui_win_list[DATA_WIN]
#define TUI_CMD_WIN     tui_win_list[CMD_WIN]

/* Data Manipulation Functions.  */
extern void tui_initialize_static_data (void);
extern struct tui_gen_win_info *tui_alloc_generic_win_info (void);
extern struct tui_win_info *tui_alloc_win_info (enum tui_win_type);
extern void tui_init_generic_part (struct tui_gen_win_info *);
extern void tui_init_win_info (struct tui_win_info *);
extern tui_win_content tui_alloc_content (int, enum tui_win_type);
extern int tui_add_content_elements (struct tui_gen_win_info *, 
				     int);
extern void tui_init_content_element (struct tui_win_element *, 
				      enum tui_win_type);
extern void tui_free_window (struct tui_win_info *);
extern void tui_free_win_content (struct tui_gen_win_info *);
extern void tui_free_data_content (tui_win_content, int);
extern void tui_free_all_source_wins_content (void);
extern void tui_del_window (struct tui_win_info *);
extern void tui_del_data_windows (tui_win_content, int);
extern struct tui_win_info *tui_partial_win_by_name (const char *);
extern const char *tui_win_name (const struct tui_gen_win_info *);
extern enum tui_layout_type tui_current_layout (void);
extern void tui_set_current_layout_to (enum tui_layout_type);
extern int tui_term_height (void);
extern void tui_set_term_height_to (int);
extern int tui_term_width (void);
extern void tui_set_term_width_to (int);
extern struct tui_gen_win_info *tui_locator_win_info_ptr (void);
extern struct tui_gen_win_info *tui_source_exec_info_win_ptr (void);
extern struct tui_gen_win_info *tui_disassem_exec_info_win_ptr (void);
extern struct tui_list *tui_source_windows (void);
extern void tui_clear_source_windows (void);
extern void tui_clear_source_windows_detail (void);
extern void tui_clear_win_detail (struct tui_win_info *);
extern void tui_add_to_source_windows (struct tui_win_info *);
extern int tui_default_tab_len (void);
extern void tui_set_default_tab_len (int);
extern struct tui_win_info *tui_win_with_focus (void);
extern void tui_set_win_with_focus (struct tui_win_info *);
extern struct tui_layout_def *tui_layout_def (void);
extern int tui_win_resized (void);
extern void tui_set_win_resized_to (int);

extern struct tui_win_info *tui_next_win (struct tui_win_info *);
extern struct tui_win_info *tui_prev_win (struct tui_win_info *);

extern void tui_add_to_source_windows (struct tui_win_info *);

#endif /* TUI_DATA_H */
