/* Output generating routines for GDB CLI.

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

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
#include "vec.h"
#include "readline/readline.h"

typedef struct cli_ui_out_data cli_out_data;

/* Prototypes for local functions */

static void cli_text (struct ui_out *uiout, const char *string);

static void field_separator (void);

static void out_field_fmt (struct ui_out *uiout, int fldno,
			   const char *fldname,
			   const char *format,...) ATTRIBUTE_PRINTF (4, 5);

/* The destructor.  */

static void
cli_uiout_dtor (struct ui_out *ui_out)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (ui_out);

  VEC_free (ui_filep, data->streams);
  xfree (data);
}

/* These are the CLI output functions */

/* Mark beginning of a table */

static void
cli_table_begin (struct ui_out *uiout, int nbrofcols,
		 int nr_rows,
		 const char *tblid)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (nr_rows == 0)
    data->suppress_output = 1;
  else
    /* Only the table suppresses the output and, fortunately, a table
       is not a recursive data structure.  */
    gdb_assert (data->suppress_output == 0);
}

/* Mark beginning of a table body */

static void
cli_table_body (struct ui_out *uiout)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;
  /* first, close the table header line */
  cli_text (uiout, "\n");
}

/* Mark end of a table */

static void
cli_table_end (struct ui_out *uiout)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  data->suppress_output = 0;
}

/* Specify table header */

static void
cli_table_header (struct ui_out *uiout, int width, enum ui_align alignment,
		  const char *col_name,
		  const char *colhdr)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  /* Always go through the function pointer (virtual function call).
     We may have been extended.  */
  uo_field_string (uiout, 0, width, alignment, 0, colhdr);
}

/* Mark beginning of a list */

static void
cli_begin (struct ui_out *uiout,
	   enum ui_out_type type,
	   int level,
	   const char *id)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;
}

/* Mark end of a list */

static void
cli_end (struct ui_out *uiout,
	 enum ui_out_type type,
	 int level)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;
}

/* output an int field */

static void
cli_field_int (struct ui_out *uiout, int fldno, int width,
	       enum ui_align alignment,
	       const char *fldname, int value)
{
  char buffer[20];	/* FIXME: how many chars long a %d can become? */
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;
  xsnprintf (buffer, sizeof (buffer), "%d", value);

  /* Always go through the function pointer (virtual function call).
     We may have been extended.  */
  uo_field_string (uiout, fldno, width, alignment, fldname, buffer);
}

/* used to ommit a field */

static void
cli_field_skip (struct ui_out *uiout, int fldno, int width,
		enum ui_align alignment,
		const char *fldname)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  /* Always go through the function pointer (virtual function call).
     We may have been extended.  */
  uo_field_string (uiout, fldno, width, alignment, fldname, "");
}

/* other specific cli_field_* end up here so alignment and field
   separators are both handled by cli_field_string */

static void
cli_field_string (struct ui_out *uiout,
		  int fldno,
		  int width,
		  enum ui_align align,
		  const char *fldname,
		  const char *string)
{
  int before = 0;
  int after = 0;
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
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
    ui_out_spaces (uiout, before);
  if (string)
    out_field_fmt (uiout, fldno, fldname, "%s", string);
  if (after)
    ui_out_spaces (uiout, after);

  if (align != ui_noalign)
    field_separator ();
}

/* This is the only field function that does not align.  */

static void ATTRIBUTE_PRINTF (6, 0)
cli_field_fmt (struct ui_out *uiout, int fldno,
	       int width, enum ui_align align,
	       const char *fldname,
	       const char *format,
	       va_list args)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);
  struct ui_file *stream;

  if (data->suppress_output)
    return;

  stream = VEC_last (ui_filep, data->streams);
  vfprintf_filtered (stream, format, args);

  if (align != ui_noalign)
    field_separator ();
}

static void
cli_spaces (struct ui_out *uiout, int numspaces)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);
  struct ui_file *stream;

  if (data->suppress_output)
    return;

  stream = VEC_last (ui_filep, data->streams);
  print_spaces_filtered (numspaces, stream);
}

static void
cli_text (struct ui_out *uiout, const char *string)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);
  struct ui_file *stream;

  if (data->suppress_output)
    return;

  stream = VEC_last (ui_filep, data->streams);
  fputs_filtered (string, stream);
}

static void ATTRIBUTE_PRINTF (3, 0)
cli_message (struct ui_out *uiout, int verbosity,
	     const char *format, va_list args)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  if (ui_out_get_verblvl (uiout) >= verbosity)
    {
      struct ui_file *stream = VEC_last (ui_filep, data->streams);

      vfprintf_unfiltered (stream, format, args);
    }
}

static void
cli_wrap_hint (struct ui_out *uiout, char *identstring)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;
  wrap_here (identstring);
}

static void
cli_flush (struct ui_out *uiout)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  gdb_flush (stream);
}

/* OUTSTREAM as non-NULL will push OUTSTREAM on the stack of output streams
   and make it therefore active.  OUTSTREAM as NULL will pop the last pushed
   output stream; it is an internal error if it does not exist.  */

static int
cli_redirect (struct ui_out *uiout, struct ui_file *outstream)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);

  if (outstream != NULL)
    VEC_safe_push (ui_filep, data->streams, outstream);
  else
    VEC_pop (ui_filep, data->streams);

  return 0;
}

/* local functions */

/* Like cli_field_fmt, but takes a variable number of args
   and makes a va_list and does not insert a separator.  */

/* VARARGS */
static void
out_field_fmt (struct ui_out *uiout, int fldno,
	       const char *fldname,
	       const char *format,...)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);
  va_list args;

  va_start (args, format);
  vfprintf_filtered (stream, format, args);

  va_end (args);
}

/* Access to ui_out format private members.  */

static void
field_separator (void)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (current_uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  fputc_filtered (' ', stream);
}

/* This is the CLI ui-out implementation functions vector */

const struct ui_out_impl cli_ui_out_impl =
{
  cli_table_begin,
  cli_table_body,
  cli_table_end,
  cli_table_header,
  cli_begin,
  cli_end,
  cli_field_int,
  cli_field_skip,
  cli_field_string,
  cli_field_fmt,
  cli_spaces,
  cli_text,
  cli_message,
  cli_wrap_hint,
  cli_flush,
  cli_redirect,
  cli_uiout_dtor,
  0, /* Does not need MI hacks (i.e. needs CLI hacks).  */
};

/* Constructor for a `cli_out_data' object.  */

void
cli_out_data_ctor (cli_out_data *self, struct ui_file *stream)
{
  gdb_assert (stream != NULL);

  self->streams = NULL;
  VEC_safe_push (ui_filep, self->streams, stream);

  self->suppress_output = 0;
}

/* Initialize private members at startup.  */

struct ui_out *
cli_out_new (struct ui_file *stream)
{
  int flags = ui_source_list;
  cli_out_data *data = XNEW (cli_out_data);

  cli_out_data_ctor (data, stream);
  return ui_out_new (&cli_ui_out_impl, data, flags);
}

struct ui_file *
cli_out_set_stream (struct ui_out *uiout, struct ui_file *stream)
{
  cli_out_data *data = (cli_out_data *) ui_out_data (uiout);
  struct ui_file *old;
  
  old = VEC_pop (ui_filep, data->streams);
  VEC_quick_push (ui_filep, data->streams, stream);

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
