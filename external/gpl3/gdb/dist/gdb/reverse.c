/* Reverse execution and reverse debugging.

   Copyright (C) 2006-2017 Free Software Foundation, Inc.

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
#include "target.h"
#include "top.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include "inferior.h"
#include "infrun.h"
#include "regcache.h"

/* User interface:
   reverse-step, reverse-next etc.  */

static void
exec_direction_default (void *notused)
{
  /* Return execution direction to default state.  */
  execution_direction = EXEC_FORWARD;
}

/* exec_reverse_once -- accepts an arbitrary gdb command (string), 
   and executes it with exec-direction set to 'reverse'.

   Used to implement reverse-next etc. commands.  */

static void
exec_reverse_once (const char *cmd, char *args, int from_tty)
{
  char *reverse_command;
  enum exec_direction_kind dir = execution_direction;
  struct cleanup *old_chain;

  if (dir == EXEC_REVERSE)
    error (_("Already in reverse mode.  Use '%s' or 'set exec-dir forward'."),
	   cmd);

  if (!target_can_execute_reverse)
    error (_("Target %s does not support this command."), target_shortname);

  reverse_command = xstrprintf ("%s %s", cmd, args ? args : "");
  old_chain = make_cleanup (exec_direction_default, NULL);
  make_cleanup (xfree, reverse_command);
  execution_direction = EXEC_REVERSE;
  execute_command (reverse_command, from_tty);
  do_cleanups (old_chain);
}

static void
reverse_step (char *args, int from_tty)
{
  exec_reverse_once ("step", args, from_tty);
}

static void
reverse_stepi (char *args, int from_tty)
{
  exec_reverse_once ("stepi", args, from_tty);
}

static void
reverse_next (char *args, int from_tty)
{
  exec_reverse_once ("next", args, from_tty);
}

static void
reverse_nexti (char *args, int from_tty)
{
  exec_reverse_once ("nexti", args, from_tty);
}

static void
reverse_continue (char *args, int from_tty)
{
  exec_reverse_once ("continue", args, from_tty);
}

static void
reverse_finish (char *args, int from_tty)
{
  exec_reverse_once ("finish", args, from_tty);
}

/* Data structures for a bookmark list.  */

struct bookmark {
  struct bookmark *next;
  int number;
  CORE_ADDR pc;
  struct symtab_and_line sal;
  gdb_byte *opaque_data;
};

static struct bookmark *bookmark_chain;
static int bookmark_count;

#define ALL_BOOKMARKS(B) for ((B) = bookmark_chain; (B); (B) = (B)->next)

#define ALL_BOOKMARKS_SAFE(B,TMP)           \
     for ((B) = bookmark_chain;             \
          (B) ? ((TMP) = (B)->next, 1) : 0; \
          (B) = (TMP))

/* save_bookmark_command -- implement "bookmark" command.
   Call target method to get a bookmark identifier.
   Insert bookmark identifier into list.

   Identifier will be a malloc string (gdb_byte *).
   Up to us to free it as required.  */

static void
save_bookmark_command (char *args, int from_tty)
{
  /* Get target's idea of a bookmark.  */
  gdb_byte *bookmark_id = target_get_bookmark (args, from_tty);
  struct bookmark *b, *b1;
  struct gdbarch *gdbarch = get_regcache_arch (get_current_regcache ());

  /* CR should not cause another identical bookmark.  */
  dont_repeat ();

  if (bookmark_id == NULL)
    error (_("target_get_bookmark failed."));

  /* Set up a bookmark struct.  */
  b = XCNEW (struct bookmark);
  b->number = ++bookmark_count;
  init_sal (&b->sal);
  b->pc = regcache_read_pc (get_current_regcache ());
  b->sal = find_pc_line (b->pc, 0);
  b->sal.pspace = get_frame_program_space (get_current_frame ());
  b->opaque_data = bookmark_id;
  b->next = NULL;

  /* Add this bookmark to the end of the chain, so that a list
     of bookmarks will come out in order of increasing numbers.  */

  b1 = bookmark_chain;
  if (b1 == 0)
    bookmark_chain = b;
  else
    {
      while (b1->next)
	b1 = b1->next;
      b1->next = b;
    }
  printf_filtered (_("Saved bookmark %d at %s\n"), b->number,
		     paddress (gdbarch, b->sal.pc));
}

/* Implement "delete bookmark" command.  */

static int
delete_one_bookmark (int num)
{
  struct bookmark *b1, *b;

  /* Find bookmark with corresponding number.  */
  ALL_BOOKMARKS (b)
    if (b->number == num)
      break;

  /* Special case, first item in list.  */
  if (b == bookmark_chain)
    bookmark_chain = b->next;

  /* Find bookmark preceding "marked" one, so we can unlink.  */
  if (b)
    {
      ALL_BOOKMARKS (b1)
	if (b1->next == b)
	  {
	    /* Found designated bookmark.  Unlink and delete.  */
	    b1->next = b->next;
	    break;
	  }
      xfree (b->opaque_data);
      xfree (b);
      return 1;		/* success */
    }
  return 0;		/* failure */
}

static void
delete_all_bookmarks (void)
{
  struct bookmark *b, *b1;

  ALL_BOOKMARKS_SAFE (b, b1)
    {
      xfree (b->opaque_data);
      xfree (b);
    }
  bookmark_chain = NULL;
}

static void
delete_bookmark_command (char *args, int from_tty)
{
  if (bookmark_chain == NULL)
    {
      warning (_("No bookmarks."));
      return;
    }

  if (args == NULL || args[0] == '\0')
    {
      if (from_tty && !query (_("Delete all bookmarks? ")))
	return;
      delete_all_bookmarks ();
      return;
    }

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();
      if (!delete_one_bookmark (num))
	/* Not found.  */
	warning (_("No bookmark #%d."), num);
    }
}

/* Implement "goto-bookmark" command.  */

static void
goto_bookmark_command (char *args, int from_tty)
{
  struct bookmark *b;
  unsigned long num;
  char *p = args;

  if (args == NULL || args[0] == '\0')
    error (_("Command requires an argument."));

  if (startswith (args, "start")
      || startswith (args, "begin")
      || startswith (args, "end"))
    {
      /* Special case.  Give target opportunity to handle.  */
      target_goto_bookmark ((gdb_byte *) args, from_tty);
      return;
    }

  if (args[0] == '\'' || args[0] == '\"')
    {
      /* Special case -- quoted string.  Pass on to target.  */
      if (args[strlen (args) - 1] != args[0])
	error (_("Unbalanced quotes: %s"), args);
      target_goto_bookmark ((gdb_byte *) args, from_tty);
      return;
    }

  /* General case.  Bookmark identified by bookmark number.  */
  num = get_number (&args);

  if (num == 0)
    error (_("goto-bookmark: invalid bookmark number '%s'."), p);

  ALL_BOOKMARKS (b)
    if (b->number == num)
      break;

  if (b)
    {
      /* Found.  Send to target method.  */
      target_goto_bookmark (b->opaque_data, from_tty);
      return;
    }
  /* Not found.  */
  error (_("goto-bookmark: no bookmark found for '%s'."), p);
}

static int
bookmark_1 (int bnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (get_current_regcache ());
  struct bookmark *b;
  int matched = 0;

  ALL_BOOKMARKS (b)
  {
    if (bnum == -1 || bnum == b->number)
      {
	printf_filtered ("   %d       %s    '%s'\n",
			 b->number,
			 paddress (gdbarch, b->pc),
			 b->opaque_data);
	matched++;
      }
  }

  if (bnum > 0 && matched == 0)
    printf_filtered ("No bookmark #%d\n", bnum);

  return matched;
}

/* Implement "info bookmarks" command.  */

static void
bookmarks_info (char *args, int from_tty)
{
  if (!bookmark_chain)
    printf_filtered (_("No bookmarks.\n"));
  else if (args == NULL || *args == '\0')
    bookmark_1 (-1);
  else
    {
      number_or_range_parser parser (args);
      while (!parser.finished ())
	{
	  int bnum = parser.get_number ();
	  bookmark_1 (bnum);
	}
    }
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_reverse;

void
_initialize_reverse (void)
{
  add_com ("reverse-step", class_run, reverse_step, _("\
Step program backward until it reaches the beginning of another source line.\n\
Argument N means do this N times (or till program stops for another reason).")
	   );
  add_com_alias ("rs", "reverse-step", class_alias, 1);

  add_com ("reverse-next", class_run, reverse_next, _("\
Step program backward, proceeding through subroutine calls.\n\
Like the \"reverse-step\" command as long as subroutine calls do not happen;\n\
when they do, the call is treated as one instruction.\n\
Argument N means do this N times (or till program stops for another reason).")
	   );
  add_com_alias ("rn", "reverse-next", class_alias, 1);

  add_com ("reverse-stepi", class_run, reverse_stepi, _("\
Step backward exactly one instruction.\n\
Argument N means do this N times (or till program stops for another reason).")
	   );
  add_com_alias ("rsi", "reverse-stepi", class_alias, 0);

  add_com ("reverse-nexti", class_run, reverse_nexti, _("\
Step backward one instruction, but proceed through called subroutines.\n\
Argument N means do this N times (or till program stops for another reason).")
	   );
  add_com_alias ("rni", "reverse-nexti", class_alias, 0);

  add_com ("reverse-continue", class_run, reverse_continue, _("\
Continue program being debugged but run it in reverse.\n\
If proceeding from breakpoint, a number N may be used as an argument,\n\
which means to set the ignore count of that breakpoint to N - 1 (so that\n\
the breakpoint won't break until the Nth time it is reached)."));
  add_com_alias ("rc", "reverse-continue", class_alias, 0);

  add_com ("reverse-finish", class_run, reverse_finish, _("\
Execute backward until just before selected stack frame is called."));

  add_com ("bookmark", class_bookmark, save_bookmark_command, _("\
Set a bookmark in the program's execution history.\n\
A bookmark represents a point in the execution history \n\
that can be returned to at a later point in the debug session."));
  add_info ("bookmarks", bookmarks_info, _("\
Status of user-settable bookmarks.\n\
Bookmarks are user-settable markers representing a point in the \n\
execution history that can be returned to later in the same debug \n\
session."));
  add_cmd ("bookmark", class_bookmark, delete_bookmark_command, _("\
Delete a bookmark from the bookmark list.\n\
Argument is a bookmark number or numbers,\n\
 or no argument to delete all bookmarks.\n"),
	   &deletelist);
  add_com ("goto-bookmark", class_bookmark, goto_bookmark_command, _("\
Go to an earlier-bookmarked point in the program's execution history.\n\
Argument is the bookmark number of a bookmark saved earlier by using \n\
the 'bookmark' command, or the special arguments:\n\
  start (beginning of recording)\n\
  end   (end of recording)\n"));
}
