/* TUI display locator.

   Copyright 1998, 1999, 2000, 2001, 2002 Free Software Foundation,
   Inc.

   Contributed by Hewlett-Packard Company.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* FIXME: cagney/2002-02-28: The GDB coding standard indicates that
   "defs.h" should be included first.  Unfortunatly some systems
   (currently Debian GNU/Linux) include the <stdbool.h> via <curses.h>
   and they clash with "bfd.h"'s definiton of true/false.  The correct
   fix is to remove true/false from "bfd.h", however, until that
   happens, hack around it by including "config.h" and <curses.h>
   first.  */

#include "config.h"
#ifdef HAVE_NCURSES_H       
#include <ncurses.h>
#else
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#endif

#include "defs.h"
#include "symtab.h"
#include "breakpoint.h"
#include "frame.h"
#include "command.h"
#include "inferior.h"
#include "target.h"
#include "top.h"

#include "tui.h"
#include "tuiData.h"
#include "tuiStack.h"
#include "tuiGeneralWin.h"
#include "tuiSource.h"
#include "tuiSourceWin.h"
#include "tui-file.h"


/* Get a printable name for the function at the address.
   The symbol name is demangled if demangling is turned on.
   Returns a pointer to a static area holding the result.  */
static char* tui_get_function_from_frame (struct frame_info *fi);

/* Set the filename portion of the locator.  */
static void tui_set_locator_filename (const char *filename);

/* Update the locator, with the provided arguments.  */
static void tui_set_locator_info (const char *filename, const char *procname,
                                  int lineno, CORE_ADDR addr);

static void tui_update_command (char *, int);


/* Create the status line to display as much information as we
   can on this single line: target name, process number, current
   function, current line, current PC, SingleKey mode.  */
static char*
tui_make_status_line (TuiLocatorElement* loc)
{
  char* string;
  char line_buf[50], *pname;
  char* buf;
  int status_size;
  int i, proc_width;
  const char* pid_name;
  const char* pc_buf;
  int target_width;
  int pid_width;
  int line_width;
  int pc_width;
  struct ui_file *pc_out;

  if (ptid_equal (inferior_ptid, null_ptid))
    pid_name = "No process";
  else
    pid_name = target_pid_to_str (inferior_ptid);

  target_width = strlen (target_shortname);
  if (target_width > MAX_TARGET_WIDTH)
    target_width = MAX_TARGET_WIDTH;

  pid_width = strlen (pid_name);
  if (pid_width > MAX_PID_WIDTH)
    pid_width = MAX_PID_WIDTH;

  status_size = termWidth ();  
  string = (char *) xmalloc (status_size + 1);
  buf = (char*) alloca (status_size + 1);

  /* Translate line number and obtain its size.  */
  if (loc->lineNo > 0)
    sprintf (line_buf, "%d", loc->lineNo);
  else
    strcpy (line_buf, "??");
  line_width = strlen (line_buf);
  if (line_width < MIN_LINE_WIDTH)
    line_width = MIN_LINE_WIDTH;

  /* Translate PC address.  */
  pc_out = tui_sfileopen (128);
  print_address_numeric (loc->addr, 1, pc_out);
  pc_buf = tui_file_get_strbuf (pc_out);
  pc_width = strlen (pc_buf);
  
  /* First determine the amount of proc name width we have available.
     The +1 are for a space separator between fields.
     The -1 are to take into account the \0 counted by sizeof.  */
  proc_width = (status_size
                - (target_width + 1)
                - (pid_width + 1)
                - (sizeof (PROC_PREFIX) - 1 + 1)
                - (sizeof (LINE_PREFIX) - 1 + line_width + 1)
                - (sizeof (PC_PREFIX) - 1 + pc_width + 1)
                - (tui_current_key_mode == tui_single_key_mode
                   ? (sizeof (SINGLE_KEY) - 1 + 1)
                   : 0));

  /* If there is no room to print the function name, try by removing
     some fields.  */
  if (proc_width < MIN_PROC_WIDTH)
    {
      proc_width += target_width + 1;
      target_width = 0;
      if (proc_width < MIN_PROC_WIDTH)
        {
          proc_width += pid_width + 1;
          pid_width = 0;
          if (proc_width <= MIN_PROC_WIDTH)
            {
              proc_width += pc_width + sizeof (PC_PREFIX) - 1 + 1;
              pc_width = 0;
              if (proc_width < 0)
                {
                  proc_width += line_width + sizeof (LINE_PREFIX) - 1 + 1;
                  line_width = 0;
                  if (proc_width < 0)
                    proc_width = 0;
                }
            }
        }
    }

  /* Now convert elements to string form */
  pname = loc->procName;

  /* Now create the locator line from the string version
     of the elements.  We could use sprintf() here but
     that wouldn't ensure that we don't overrun the size
     of the allocated buffer.  strcat_to_buf() will.  */
  *string = (char) 0;

  if (target_width > 0)
    {
      sprintf (buf, "%*.*s ",
               -target_width, target_width, target_shortname);
      strcat_to_buf (string, status_size, buf);
    }
  if (pid_width > 0)
    {
      sprintf (buf, "%*.*s ",
               -pid_width, pid_width, pid_name);
      strcat_to_buf (string, status_size, buf);
    }
  
  /* Show whether we are in SingleKey mode.  */
  if (tui_current_key_mode == tui_single_key_mode)
    {
      strcat_to_buf (string, status_size, SINGLE_KEY);
      strcat_to_buf (string, status_size, " ");
    }

  /* procedure/class name */
  if (proc_width > 0)
    {
      if (strlen (pname) > proc_width)
        sprintf (buf, "%s%*.*s* ", PROC_PREFIX,
                 1 - proc_width, proc_width - 1, pname);
      else
        sprintf (buf, "%s%*.*s ", PROC_PREFIX,
                 -proc_width, proc_width, pname);
      strcat_to_buf (string, status_size, buf);
    }

  if (line_width > 0)
    {
      sprintf (buf, "%s%*.*s ", LINE_PREFIX,
               -line_width, line_width, line_buf);
      strcat_to_buf (string, status_size, buf);
    }
  if (pc_width > 0)
    {
      strcat_to_buf (string, status_size, PC_PREFIX);
      strcat_to_buf (string, status_size, pc_buf);
    }
  
  
  for (i = strlen (string); i < status_size; i++)
    string[i] = ' ';
  string[status_size] = (char) 0;

  ui_file_delete (pc_out);
  return string;
}

/* Get a printable name for the function at the address.
   The symbol name is demangled if demangling is turned on.
   Returns a pointer to a static area holding the result.  */
static char*
tui_get_function_from_frame (struct frame_info *fi)
{
  static char name[256];
  struct ui_file *stream = tui_sfileopen (256);
  char *p;

  print_address_symbolic (fi->pc, stream, demangle, "");
  p = tui_file_get_strbuf (stream);

  /* Use simple heuristics to isolate the function name.  The symbol can
     be demangled and we can have function parameters.  Remove them because
     the status line is too short to display them.  */
  if (*p == '<')
    p++;
  strncpy (name, p, sizeof (name));
  p = strchr (name, '(');
  if (!p)
    p = strchr (name, '>');
  if (p)
    *p = 0;
  p = strchr (name, '+');
  if (p)
    *p = 0;
  ui_file_delete (stream);
  return name;
}

/*
   ** tuiShowLocatorContent()
 */
void
tuiShowLocatorContent (void)
{
  char *string;
  TuiGenWinInfoPtr locator;

  locator = locatorWinInfoPtr ();

  if (m_genWinPtrNotNull (locator) && locator->handle != (WINDOW *) NULL)
    {
      TuiWinElementPtr element;

      element = (TuiWinElementPtr) locator->content[0];

      string = tui_make_status_line (&element->whichElement.locator);
      wmove (locator->handle, 0, 0);
      wstandout (locator->handle);
      waddstr (locator->handle, string);
      wclrtoeol (locator->handle);
      wstandend (locator->handle);
      tuiRefreshWin (locator);
      wmove (locator->handle, 0, 0);
      xfree (string);
      locator->contentInUse = TRUE;
    }
}


/* Set the filename portion of the locator.  */
static void
tui_set_locator_filename (const char *filename)
{
  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
  TuiLocatorElementPtr element;

  if (locator->content[0] == (Opaque) NULL)
    {
      tui_set_locator_info (filename, NULL, 0, 0);
      return;
    }

  element = &((TuiWinElementPtr) locator->content[0])->whichElement.locator;
  element->fileName[0] = 0;
  strcat_to_buf (element->fileName, MAX_LOCATOR_ELEMENT_LEN, filename);
}

/* Update the locator, with the provided arguments.  */
static void
tui_set_locator_info (const char *filename, const char *procname, int lineno,
                      CORE_ADDR addr)
{
  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
  TuiLocatorElementPtr element;

  /* Allocate the locator content if necessary.  */
  if (locator->contentSize <= 0)
    {
      locator->content = (OpaquePtr) allocContent (1, locator->type);
      locator->contentSize = 1;
    }

  element = &((TuiWinElementPtr) locator->content[0])->whichElement.locator;
  element->procName[0] = (char) 0;
  strcat_to_buf (element->procName, MAX_LOCATOR_ELEMENT_LEN, procname);
  element->lineNo = lineno;
  element->addr = addr;
  tui_set_locator_filename (filename);
}

/* Update only the filename portion of the locator.  */
void
tuiUpdateLocatorFilename (const char *filename)
{
  tui_set_locator_filename (filename);
  tuiShowLocatorContent ();
}

/* Function to print the frame information for the TUI.  */
void
tuiShowFrameInfo (struct frame_info *fi)
{
  TuiWinInfoPtr winInfo;
  register int i;

  if (fi)
    {
      register int startLine, i;
      CORE_ADDR low;
      TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
      int sourceAlreadyDisplayed;
      struct symtab_and_line sal;

      sal = find_pc_line (fi->pc,
                          (fi->next != (struct frame_info *) NULL &&
                           !fi->next->signal_handler_caller &&
                           !frame_in_dummy (fi->next)));

      sourceAlreadyDisplayed = sal.symtab != 0
        && tuiSourceIsDisplayed (sal.symtab->filename);
      tui_set_locator_info (sal.symtab == 0 ? "??" : sal.symtab->filename,
                            tui_get_function_from_frame (fi),
                            sal.line,
                            fi->pc);
      tuiShowLocatorContent ();
      startLine = 0;
      for (i = 0; i < (sourceWindows ())->count; i++)
	{
	  TuiWhichElement *item;
	  winInfo = (TuiWinInfoPtr) (sourceWindows ())->list[i];

	  item = &((TuiWinElementPtr) locator->content[0])->whichElement;
	  if (winInfo == srcWin)
	    {
	      startLine = (item->locator.lineNo -
			   (winInfo->generic.viewportHeight / 2)) + 1;
	      if (startLine <= 0)
		startLine = 1;
	    }
	  else
	    {
	      if (find_pc_partial_function (fi->pc, (char **) NULL, &low, (CORE_ADDR) NULL) == 0)
		error ("No function contains program counter for selected frame.\n");
	      else
		low = tuiGetLowDisassemblyAddress (low, fi->pc);
	    }

	  if (winInfo == srcWin)
	    {
	      TuiLineOrAddress l;
	      l.lineNo = startLine;
	      if (!(sourceAlreadyDisplayed
		    && tuiLineIsDisplayed (item->locator.lineNo, winInfo, TRUE)))
		tuiUpdateSourceWindow (winInfo, sal.symtab, l, TRUE);
	      else
		{
		  l.lineNo = item->locator.lineNo;
		  tuiSetIsExecPointAt (l, winInfo);
		}
	    }
	  else
	    {
	      if (winInfo == disassemWin)
		{
		  TuiLineOrAddress a;
		  a.addr = low;
		  if (!tuiAddrIsDisplayed (item->locator.addr, winInfo, TRUE))
		    tuiUpdateSourceWindow (winInfo, sal.symtab, a, TRUE);
		  else
		    {
		      a.addr = item->locator.addr;
		      tuiSetIsExecPointAt (a, winInfo);
		    }
		}
	    }
	  tuiUpdateExecInfo (winInfo);
	}
    }
  else
    {
      tui_set_locator_info (NULL, NULL, 0, (CORE_ADDR) 0);
      tuiShowLocatorContent ();
      for (i = 0; i < (sourceWindows ())->count; i++)
	{
	  winInfo = (TuiWinInfoPtr) (sourceWindows ())->list[i];
	  tuiClearSourceContent (winInfo, EMPTY_SOURCE_PROMPT);
	  tuiUpdateExecInfo (winInfo);
	}
    }
}

/* Function to initialize gdb commands, for tui window stack manipulation.  */
void
_initialize_tuiStack (void)
{
  add_com ("update", class_tui, tui_update_command,
           "Update the source window and locator to display the current "
           "execution point.\n");
}

/* Command to update the display with the current execution point.  */
static void
tui_update_command (char *arg, int from_tty)
{
  char cmd[sizeof("frame 0")];

  strcpy (cmd, "frame 0");
  execute_command (cmd, from_tty);
}
