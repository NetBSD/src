/* MI Command Set - output generating routines.
   Copyright (C) 2000, Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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

#include "defs.h"
#include "ui-out.h"
#include "mi-out.h"

/* Convenience macro for allocting typesafe memory. */

#ifndef XMALLOC
#define XMALLOC(TYPE) (TYPE*) xmalloc (sizeof (TYPE))
#endif

struct ui_out_data
  {
    int supress_field_separator;
    int first_header;
    struct ui_file *buffer;
  };

/* These are the MI output functions */

static void mi_table_begin (struct ui_out *uiout, int nbrofcols, char *tblid);
static void mi_table_body (struct ui_out *uiout);
static void mi_table_end (struct ui_out *uiout);
static void mi_table_header (struct ui_out *uiout, int width,
			     enum ui_align alig, char *colhdr);
static void mi_list_begin (struct ui_out *uiout, int list_flag, char *lstid);
static void mi_list_end (struct ui_out *uiout, int list_flag);
static void mi_field_int (struct ui_out *uiout, int fldno, int width,
			  enum ui_align alig, char *fldname, int value);
static void mi_field_skip (struct ui_out *uiout, int fldno, int width,
			   enum ui_align alig, char *fldname);
static void mi_field_string (struct ui_out *uiout, int fldno, int width,
			     enum ui_align alig, char *fldname,
			     const char *string);
static void mi_field_fmt (struct ui_out *uiout, int fldno,
			  int width, enum ui_align align,
			  char *fldname, char *format, va_list args);
static void mi_spaces (struct ui_out *uiout, int numspaces);
static void mi_text (struct ui_out *uiout, char *string);
static void mi_message (struct ui_out *uiout, int verbosity, char *format,
			va_list args);
static void mi_wrap_hint (struct ui_out *uiout, char *identstring);
static void mi_flush (struct ui_out *uiout);

/* This is the MI ui-out implementation functions vector */

/* FIXME: This can be initialized dynamically after default is set to
   handle initial output in main.c */

struct ui_out_impl mi_ui_out_impl =
{
  mi_table_begin,
  mi_table_body,
  mi_table_end,
  mi_table_header,
  mi_list_begin,
  mi_list_end,
  mi_field_int,
  mi_field_skip,
  mi_field_string,
  mi_field_fmt,
  mi_spaces,
  mi_text,
  mi_message,
  mi_wrap_hint,
  mi_flush
};

/* Prototypes for local functions */

extern void _initialize_mi_out PARAMS ((void));
static void field_separator (struct ui_out *uiout);
static void list_open (struct ui_out *uiout);
static void list_close (struct ui_out *uiout);

static void out_field_fmt (struct ui_out *uiout, int fldno, char *fldname,
			   char *format,...);

/* Mark beginning of a table */

void
mi_table_begin (uiout, nbrofcols, tblid)
     struct ui_out *uiout;
     int nbrofcols;
     char *tblid;
{
  struct ui_out_data *data = ui_out_data (uiout);
  field_separator (uiout);
  if (tblid)
    fprintf_unfiltered (data->buffer, "%s=", tblid);
  list_open (uiout);
  data->first_header = 0;
  data->supress_field_separator = 1;
}

/* Mark beginning of a table body */

void
mi_table_body (uiout)
     struct ui_out *uiout;
{
  struct ui_out_data *data = ui_out_data (uiout);
  /* close the table header line if there were any headers */
  if (data->first_header)
    list_close (uiout);
}

/* Mark end of a table */

void
mi_table_end (uiout)
     struct ui_out *uiout;
{
  struct ui_out_data *data = ui_out_data (uiout);
  list_close (uiout);
  /* If table was empty this flag did not get reset yet */
  data->supress_field_separator = 0;
}

/* Specify table header */

void
mi_table_header (uiout, width, alignment, colhdr)
     struct ui_out *uiout;
     int width;
     int alignment;
     char *colhdr;
{
  struct ui_out_data *data = ui_out_data (uiout);
  if (!data->first_header++)
    {
      fputs_unfiltered ("hdr=", data->buffer);
      list_open (uiout);
    }
  mi_field_string (uiout, 0, width, alignment, 0, colhdr);
}

/* Mark beginning of a list */

void
mi_list_begin (uiout, list_flag, lstid)
     struct ui_out *uiout;
     int list_flag;
     char *lstid;
{
  struct ui_out_data *data = ui_out_data (uiout);
  field_separator (uiout);
  data->supress_field_separator = 1;
  if (lstid)
    fprintf_unfiltered (data->buffer, "%s=", lstid);
  list_open (uiout);
}

/* Mark end of a list */

void
mi_list_end (uiout, list_flag)
     struct ui_out *uiout;
     int list_flag;
{
  struct ui_out_data *data = ui_out_data (uiout);
  list_close (uiout);
  /* If list was empty this flag did not get reset yet */
  data->supress_field_separator = 0;
}

/* output an int field */

void
mi_field_int (uiout, fldno, width, alignment, fldname, value)
     struct ui_out *uiout;
     int fldno;
     int width;
     int alignment;
     char *fldname;
     int value;
{
  char buffer[20];		/* FIXME: how many chars long a %d can become? */

  sprintf (buffer, "%d", value);
  mi_field_string (uiout, fldno, width, alignment, fldname, buffer);
}

/* used to ommit a field */

void
mi_field_skip (uiout, fldno, width, alignment, fldname)
     struct ui_out *uiout;
     int fldno;
     int width;
     int alignment;
     char *fldname;
{
  mi_field_string (uiout, fldno, width, alignment, fldname, "");
}

/* other specific mi_field_* end up here so alignment and field
   separators are both handled by mi_field_string */

void
mi_field_string (struct ui_out *uiout,
		 int fldno,
		 int width,
		 int align,
		 char *fldname,
		 const char *string)
{
  struct ui_out_data *data = ui_out_data (uiout);
  field_separator (uiout);
  if (fldname)
    fprintf_unfiltered (data->buffer, "%s=", fldname);
  fprintf_unfiltered (data->buffer, "\"");
  if (string)
    fputstr_unfiltered (string, '"', data->buffer);
  fprintf_unfiltered (data->buffer, "\"");
}

/* This is the only field function that does not align */

void
mi_field_fmt (struct ui_out *uiout, int fldno,
	      int width, enum ui_align align,
	      char *fldname, char *format, va_list args)
{
  struct ui_out_data *data = ui_out_data (uiout);
  field_separator (uiout);
  if (fldname)
    fprintf_unfiltered (data->buffer, "%s=\"", fldname);
  else
    fputs_unfiltered ("\"", data->buffer);
  vfprintf_unfiltered (data->buffer, format, args);
  fputs_unfiltered ("\"", data->buffer);
}

void
mi_spaces (uiout, numspaces)
     struct ui_out *uiout;
     int numspaces;
{
}

void
mi_text (uiout, string)
     struct ui_out *uiout;
     char *string;
{
}

void
mi_message (struct ui_out *uiout, int verbosity, char *format, va_list args)
{
}

void
mi_wrap_hint (uiout, identstring)
     struct ui_out *uiout;
     char *identstring;
{
  wrap_here (identstring);
}

void
mi_flush (uiout)
     struct ui_out *uiout;
{
  struct ui_out_data *data = ui_out_data (uiout);
  gdb_flush (data->buffer);
}

/* local functions */

/* Like mi_field_fmt, but takes a variable number of args
   and makes a va_list and does not insert a separator */

/* VARARGS */
static void
out_field_fmt (struct ui_out *uiout, int fldno, char *fldname,
	       char *format,...)
{
  struct ui_out_data *data = ui_out_data (uiout);
  va_list args;

  field_separator (uiout);
  if (fldname)
    fprintf_unfiltered (data->buffer, "%s=\"", fldname);
  else
    fputs_unfiltered ("\"", data->buffer);

  va_start (args, format);
  vfprintf_unfiltered (data->buffer, format, args);

  fputs_unfiltered ("\"", data->buffer);

  va_end (args);
}

/* access to ui_out format private members */

static void
field_separator (struct ui_out *uiout)
{
  struct ui_out_data *data = ui_out_data (uiout);
  if (data->supress_field_separator)
    data->supress_field_separator = 0;
  else
    fputc_unfiltered (',', data->buffer);
}

static void
list_open (struct ui_out *uiout)
{
  struct ui_out_data *data = ui_out_data (uiout);
  fputc_unfiltered ('{', data->buffer);
}

static void
list_close (struct ui_out *uiout)
{
  struct ui_out_data *data = ui_out_data (uiout);
  fputc_unfiltered ('}', data->buffer);
}

/* add a string to the buffer */

void
mi_out_buffered (struct ui_out *uiout, char *string)
{
  struct ui_out_data *data = ui_out_data (uiout);
  fprintf_unfiltered (data->buffer, "%s", string);
}

/* clear the buffer */

void
mi_out_rewind (struct ui_out *uiout)
{
  struct ui_out_data *data = ui_out_data (uiout);
  ui_file_rewind (data->buffer);
}

/* dump the buffer onto the specified stream */

static void
do_write (void *data, const char *buffer, long length_buffer)
{
  ui_file_write (data, buffer, length_buffer);
}

void
mi_out_put (struct ui_out *uiout,
	    struct ui_file *stream)
{
  struct ui_out_data *data = ui_out_data (uiout);
  ui_file_put (data->buffer, do_write, stream);
  ui_file_rewind (data->buffer);
}

/* initalize private members at startup */

struct ui_out *
mi_out_new (void)
{
  int flags = 0;
  struct ui_out_data *data = XMALLOC (struct ui_out_data);
  data->supress_field_separator = 0;
  /* FIXME: This code should be using a ``string_file'' and not the
     TUI buffer hack. */
  data->buffer = mem_fileopen ();
  return ui_out_new (&mi_ui_out_impl, data, flags);
}

/* standard gdb initialization hook */
void
_initialize_mi_out ()
{
  /* nothing happens here */
}

/* Local variables: */
/* change-log-default-name: "ChangeLog-mi" */
/* End: */
