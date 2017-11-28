/* Support for complaint handling during symbol reading in GDB.

   Copyright (C) 1990-2016 Free Software Foundation, Inc.

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
#include "complaints.h"
#include "command.h"
#include "gdbcmd.h"

extern void _initialize_complaints (void);

/* Should each complaint message be self explanatory, or should we
   assume that a series of complaints is being produced?  */

enum complaint_series {
  /* Isolated self explanatory message.  */
  ISOLATED_MESSAGE,

  /* First message of a series, includes an explanation.  */
  FIRST_MESSAGE,

  /* First message of a series, but does not need to include any sort
     of explanation.  */
  SHORT_FIRST_MESSAGE,

  /* Subsequent message of a series that needs no explanation (the
     user already knows we have a problem so we can just state our
     piece).  */
  SUBSEQUENT_MESSAGE
};

/* Structure to manage complaints about symbol file contents.  */

struct complain
{
  const char *file;
  int line;
  const char *fmt;
  int counter;
  struct complain *next;
};

/* The explanatory message that should accompany the complaint.  The
   message is in two parts - pre and post - that are printed around
   the complaint text.  */
struct explanation
{
  const char *prefix;
  const char *postfix;
};

struct complaints
{
  struct complain *root;

  enum complaint_series series;

  /* The explanatory messages that should accompany the complaint.
     NOTE: cagney/2002-08-14: In a desperate attempt at being vaguely
     i18n friendly, this is an array of two messages.  When present,
     the PRE and POST EXPLANATION[SERIES] are used to wrap the
     message.  */
  const struct explanation *explanation;
};

static struct complain complaint_sentinel;

/* The symbol table complaint table.  */

static struct explanation symfile_explanations[] = {
  { "During symbol reading, ", "." },
  { "During symbol reading...", "..."},
  { "", "..."},
  { "", "..."},
  { NULL, NULL }
};

static struct complaints symfile_complaint_book = {
  &complaint_sentinel,
  ISOLATED_MESSAGE,
  symfile_explanations
};
struct complaints *symfile_complaints = &symfile_complaint_book;

/* Wrapper function to, on-demand, fill in a complaints object.  */

static struct complaints *
get_complaints (struct complaints **c)
{
  if ((*c) != NULL)
    return (*c);
  (*c) = XNEW (struct complaints);
  (*c)->root = &complaint_sentinel;
  (*c)->series = ISOLATED_MESSAGE;
  (*c)->explanation = NULL;
  return (*c);
}

static struct complain * ATTRIBUTE_PRINTF (4, 0)
find_complaint (struct complaints *complaints, const char *file,
		int line, const char *fmt)
{
  struct complain *complaint;

  /* Find the complaint in the table.  A more efficient search
     algorithm (based on hash table or something) could be used.  But
     that can wait until someone shows evidence that this lookup is
     a real bottle neck.  */
  for (complaint = complaints->root;
       complaint != NULL;
       complaint = complaint->next)
    {
      if (complaint->fmt == fmt
	  && complaint->file == file
	  && complaint->line == line)
	return complaint;
    }

  /* Oops not seen before, fill in a new complaint.  */
  complaint = XNEW (struct complain);
  complaint->fmt = fmt;
  complaint->file = file;
  complaint->line = line;
  complaint->counter = 0;
  complaint->next = NULL;

  /* File it, return it.  */
  complaint->next = complaints->root;
  complaints->root = complaint;
  return complaint;
}


/* How many complaints about a particular thing should be printed
   before we stop whining about it?  Default is no whining at all,
   since so many systems have ill-constructed symbol files.  */

static int stop_whining = 0;

/* Print a complaint, and link the complaint block into a chain for
   later handling.  */

static void ATTRIBUTE_PRINTF (4, 0)
vcomplaint (struct complaints **c, const char *file, 
	    int line, const char *fmt,
	    va_list args)
{
  struct complaints *complaints = get_complaints (c);
  struct complain *complaint = find_complaint (complaints, file, 
					       line, fmt);
  enum complaint_series series;

  gdb_assert (complaints != NULL);

  complaint->counter++;
  if (complaint->counter > stop_whining)
    return;

  if (info_verbose)
    series = SUBSEQUENT_MESSAGE;
  else
    series = complaints->series;

  /* Pass 'fmt' instead of 'complaint->fmt' to printf-like callees
     from here on, to avoid "format string is not a string literal"
     warnings.  'fmt' is this function's printf-format parameter, so
     the compiler can assume the passed in argument is a literal
     string somewhere up the call chain.  */
  gdb_assert (complaint->fmt == fmt);

  if (complaint->file != NULL)
    internal_vwarning (complaint->file, complaint->line, fmt, args);
  else if (deprecated_warning_hook)
    (*deprecated_warning_hook) (fmt, args);
  else
    {
      if (complaints->explanation == NULL)
	/* A [v]warning() call always appends a newline.  */
	vwarning (fmt, args);
      else
	{
	  char *msg;
	  struct cleanup *cleanups;
	  msg = xstrvprintf (fmt, args);
	  cleanups = make_cleanup (xfree, msg);
	  wrap_here ("");
	  if (series != SUBSEQUENT_MESSAGE)
	    begin_line ();
	  /* XXX: i18n */
	  fprintf_filtered (gdb_stderr, "%s%s%s",
			    complaints->explanation[series].prefix, msg,
			    complaints->explanation[series].postfix);
	  /* Force a line-break after any isolated message.  For the
             other cases, clear_complaints() takes care of any missing
             trailing newline, the wrap_here() is just a hint.  */
	  if (series == ISOLATED_MESSAGE)
	    /* It would be really nice to use begin_line() here.
	       Unfortunately that function doesn't track GDB_STDERR and
	       consequently will sometimes supress a line when it
	       shouldn't.  */
	    fputs_filtered ("\n", gdb_stderr);
	  else
	    wrap_here ("");
	  do_cleanups (cleanups);
	}
    }

  switch (series)
    {
    case ISOLATED_MESSAGE:
      break;
    case FIRST_MESSAGE:
      complaints->series = SUBSEQUENT_MESSAGE;
      break;
    case SUBSEQUENT_MESSAGE:
    case SHORT_FIRST_MESSAGE:
      complaints->series = SUBSEQUENT_MESSAGE;
      break;
    }

  /* If GDB dumps core, we'd like to see the complaints first.
     Presumably GDB will not be sending so many complaints that this
     becomes a performance hog.  */

  gdb_flush (gdb_stderr);
}

void
complaint (struct complaints **complaints, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vcomplaint (complaints, NULL/*file*/, 0/*line*/, fmt, args);
  va_end (args);
}

void
internal_complaint (struct complaints **complaints, const char *file,
		    int line, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  vcomplaint (complaints, file, line, fmt, args);
  va_end (args);
}

/* Clear out / initialize all complaint counters that have ever been
   incremented.  If LESS_VERBOSE is 1, be less verbose about
   successive complaints, since the messages are appearing all
   together during a command that is reporting a contiguous block of
   complaints (rather than being interleaved with other messages).  If
   noisy is 1, we are in a noisy command, and our caller will print
   enough context for the user to figure it out.  */

void
clear_complaints (struct complaints **c, int less_verbose, int noisy)
{
  struct complaints *complaints = get_complaints (c);
  struct complain *p;

  for (p = complaints->root; p != NULL; p = p->next)
    {
      p->counter = 0;
    }

  switch (complaints->series)
    {
    case FIRST_MESSAGE:
      /* Haven't yet printed anything.  */
      break;
    case SHORT_FIRST_MESSAGE:
      /* Haven't yet printed anything.  */
      break;
    case ISOLATED_MESSAGE:
      /* The code above, always forces a line-break.  No need to do it
         here.  */
      break;
    case SUBSEQUENT_MESSAGE:
      /* It would be really nice to use begin_line() here.
         Unfortunately that function doesn't track GDB_STDERR and
         consequently will sometimes supress a line when it
         shouldn't.  */
      fputs_unfiltered ("\n", gdb_stderr);
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }

  if (!less_verbose)
    complaints->series = ISOLATED_MESSAGE;
  else if (!noisy)
    complaints->series = FIRST_MESSAGE;
  else
    complaints->series = SHORT_FIRST_MESSAGE;
}

static void
complaints_show_value (struct ui_file *file, int from_tty,
		       struct cmd_list_element *cmd, const char *value)
{
  fprintf_filtered (file, _("Max number of complaints about incorrect"
			    " symbols is %s.\n"),
		    value);
}

void
_initialize_complaints (void)
{
  add_setshow_zinteger_cmd ("complaints", class_support, 
			    &stop_whining, _("\
Set max number of complaints about incorrect symbols."), _("\
Show max number of complaints about incorrect symbols."), NULL,
			    NULL, complaints_show_value,
			    &setlist, &showlist);
}
