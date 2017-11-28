/* Output generating routines for GDB CLI.

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions.
   Written by Fernando Nasser for Cygnus.

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
#include "ui-out.h"
#include "cli-out.h"
#include "completer.h"
#include "readline/readline.h"

/* These are the CLI output functions */

/* Mark beginning of a table */

void
cli_ui_out::do_table_begin (int nbrofcols, int nr_rows, const char *tblid)
{
  if (nr_rows == 0)
    m_suppress_output = true;
  else
    /* Only the table suppresses the output and, fortunately, a table
       is not a recursive data structure.  */
    gdb_assert (!m_suppress_output);
}

/* Mark beginning of a table body */

void
cli_ui_out::do_table_body ()
{
  if (m_suppress_output)
    return;

  /* first, close the table header line */
  text ("\n");
}

/* Mark end of a table */

void
cli_ui_out::do_table_end ()
{
  m_suppress_output = false;
}

/* Specify table header */

void
cli_ui_out::do_table_header (int width, ui_align alignment,
			     const std::string &col_name,
			     const std::string &col_hdr)
{
  if (m_suppress_output)
    return;

  do_field_string (0, width, alignment, 0, col_hdr.c_str ());
}

/* Mark beginning of a list */

void
cli_ui_out::do_begin (ui_out_type type, const char *id)
{
}

/* Mark end of a list */

void
cli_ui_out::do_end (ui_out_type type)
{
}

/* output an int field */

void
cli_ui_out::do_field_int (int fldno, int width, ui_align alignment,
			  const char *fldname, int value)
{
  char buffer[20];	/* FIXME: how many chars long a %d can become? */

  if (m_suppress_output)
    return;

  xsnprintf (buffer, sizeof (buffer), "%d", value);

  do_field_string (fldno, width, alignment, fldname, buffer);
}

/* used to omit a field */

void
cli_ui_out::do_field_skip (int fldno, int width, ui_align alignment,
			   const char *fldname)
{
  if (m_suppress_output)
    return;

  do_field_string (fldno, width, alignment, fldname, "");
}

/* other specific cli_field_* end up here so alignment and field
   separators are both handled by cli_field_string */

void
cli_ui_out::do_field_string (int fldno, int width, ui_align align,
			     const char *fldname, const char *string)
{
  int before = 0;
  int after = 0;

  if (m_suppress_output)
    return;

  if ((align != ui_noalign) && string)
    {
      before = width - strlen (string);
      if (before <= 0)
	before = 0;
      else
	{
	  if (align == ui_right)
	    after = 0;
	  else if (align == ui_left)
	    {
	      after = before;
	      before = 0;
	    }
	  else
	    /* ui_center */
	    {
	      after = before / 2;
	      before -= after;
	    }
	}
    }

  if (before)
    spaces (before);

  if (string)
    out_field_fmt (fldno, fldname, "%s", string);

  if (after)
    spaces (after);

  if (align != ui_noalign)
    field_separator ();
}

/* This is the only field function that does not align.  */

void
cli_ui_out::do_field_fmt (int fldno, int width, ui_align align,
			  const char *fldname, const char *format,
			  va_list args)
{
  if (m_suppress_output)
    return;

  vfprintf_filtered (m_streams.back (), format, args);

  if (align != ui_noalign)
    field_separator ();
}

void
cli_ui_out::do_spaces (int numspaces)
{
  if (m_suppress_output)
    return;

  print_spaces_filtered (numspaces, m_streams.back ());
}

void
cli_ui_out::do_text (const char *string)
{
  if (m_suppress_output)
    return;

  fputs_filtered (string, m_streams.back ());
}

void
cli_ui_out::do_message (const char *format, va_list args)
{
  if (m_suppress_output)
    return;

  vfprintf_unfiltered (m_streams.back (), format, args);
}

void
cli_ui_out::do_wrap_hint (const char *identstring)
{
  if (m_suppress_output)
    return;

  wrap_here (identstring);
}

void
cli_ui_out::do_flush ()
{
  gdb_flush (m_streams.back ());
}

/* OUTSTREAM as non-NULL will push OUTSTREAM on the stack of output streams
   and make it therefore active.  OUTSTREAM as NULL will pop the last pushed
   output stream; it is an internal error if it does not exist.  */

void
cli_ui_out::do_redirect (ui_file *outstream)
{
  if (outstream != NULL)
    m_streams.push_back (outstream);
  else
    m_streams.pop_back ();
}

/* local functions */

/* Like cli_ui_out::do_field_fmt, but takes a variable number of args
   and makes a va_list and does not insert a separator.  */

/* VARARGS */
void
cli_ui_out::out_field_fmt (int fldno, const char *fldname,
			   const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_filtered (m_streams.back (), format, args);

  va_end (args);
}

void
cli_ui_out::field_separator ()
{
  fputc_filtered (' ', m_streams.back ());
}

/* Constructor for cli_ui_out.  */

cli_ui_out::cli_ui_out (ui_file *stream, ui_out_flags flags)
: ui_out (flags),
  m_suppress_output (false)
{
  gdb_assert (stream != NULL);

  m_streams.push_back (stream);
}

cli_ui_out::~cli_ui_out ()
{
}

/* Initialize private members at startup.  */

cli_ui_out *
cli_out_new (struct ui_file *stream)
{
  return new cli_ui_out (stream, ui_source_list);
}

ui_file *
cli_ui_out::set_stream (struct ui_file *stream)
{
  ui_file *old;

  old = m_streams.back ();
  m_streams.back () = stream;

  return old;
}

/* CLI interface to display tab-completion matches.  */

/* CLI version of displayer.crlf.  */

static void
cli_mld_crlf (const struct match_list_displayer *displayer)
{
  rl_crlf ();
}

/* CLI version of displayer.putch.  */

static void
cli_mld_putch (const struct match_list_displayer *displayer, int ch)
{
  putc (ch, rl_outstream);
}

/* CLI version of displayer.puts.  */

static void
cli_mld_puts (const struct match_list_displayer *displayer, const char *s)
{
  fputs (s, rl_outstream);
}

/* CLI version of displayer.flush.  */

static void
cli_mld_flush (const struct match_list_displayer *displayer)
{
  fflush (rl_outstream);
}

EXTERN_C void _rl_erase_entire_line (void);

/* CLI version of displayer.erase_entire_line.  */

static void
cli_mld_erase_entire_line (const struct match_list_displayer *displayer)
{
  _rl_erase_entire_line ();
}

/* CLI version of displayer.beep.  */

static void
cli_mld_beep (const struct match_list_displayer *displayer)
{
  rl_ding ();
}

/* CLI version of displayer.read_key.  */

static int
cli_mld_read_key (const struct match_list_displayer *displayer)
{
  return rl_read_key ();
}

/* CLI version of rl_completion_display_matches_hook.
   See gdb_display_match_list for a description of the arguments.  */

void
cli_display_match_list (char **matches, int len, int max)
{
  struct match_list_displayer displayer;

  rl_get_screen_size (&displayer.height, &displayer.width);
  displayer.crlf = cli_mld_crlf;
  displayer.putch = cli_mld_putch;
  displayer.puts = cli_mld_puts;
  displayer.flush = cli_mld_flush;
  displayer.erase_entire_line = cli_mld_erase_entire_line;
  displayer.beep = cli_mld_beep;
  displayer.read_key = cli_mld_read_key;

  gdb_display_match_list (matches, len, max, &displayer);
  rl_forced_update_display ();
}
