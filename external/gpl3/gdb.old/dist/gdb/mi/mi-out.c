/* MI Command Set - output generating routines.

   Copyright (C) 2000-2016 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "mi-out.h"
#include "vec.h"

typedef struct ui_file *ui_filep;
DEF_VEC_P (ui_filep);

struct ui_out_data
  {
    int suppress_field_separator;
    int suppress_output;
    int mi_version;
    VEC (ui_filep) *streams;
  };
typedef struct ui_out_data mi_out_data;

/* These are the MI output functions */

static void mi_out_data_dtor (struct ui_out *ui_out);
static void mi_table_begin (struct ui_out *uiout, int nbrofcols,
			    int nr_rows, const char *tblid);
static void mi_table_body (struct ui_out *uiout);
static void mi_table_end (struct ui_out *uiout);
static void mi_table_header (struct ui_out *uiout, int width,
			     enum ui_align alig, const char *col_name,
			     const char *colhdr);
static void mi_begin (struct ui_out *uiout, enum ui_out_type type,
		      int level, const char *id);
static void mi_end (struct ui_out *uiout, enum ui_out_type type, int level);
static void mi_field_int (struct ui_out *uiout, int fldno, int width,
			  enum ui_align alig, const char *fldname, int value);
static void mi_field_skip (struct ui_out *uiout, int fldno, int width,
			   enum ui_align alig, const char *fldname);
static void mi_field_string (struct ui_out *uiout, int fldno, int width,
			     enum ui_align alig, const char *fldname,
			     const char *string);
static void mi_field_fmt (struct ui_out *uiout, int fldno,
			  int width, enum ui_align align,
			  const char *fldname, const char *format,
			  va_list args) ATTRIBUTE_PRINTF (6, 0);
static void mi_spaces (struct ui_out *uiout, int numspaces);
static void mi_text (struct ui_out *uiout, const char *string);
static void mi_message (struct ui_out *uiout, int verbosity,
			const char *format, va_list args)
     ATTRIBUTE_PRINTF (3, 0);
static void mi_wrap_hint (struct ui_out *uiout, char *identstring);
static void mi_flush (struct ui_out *uiout);
static int mi_redirect (struct ui_out *uiout, struct ui_file *outstream);

/* This is the MI ui-out implementation functions vector */

static const struct ui_out_impl mi_ui_out_impl =
{
  mi_table_begin,
  mi_table_body,
  mi_table_end,
  mi_table_header,
  mi_begin,
  mi_end,
  mi_field_int,
  mi_field_skip,
  mi_field_string,
  mi_field_fmt,
  mi_spaces,
  mi_text,
  mi_message,
  mi_wrap_hint,
  mi_flush,
  mi_redirect,
  mi_out_data_dtor,
  1, /* Needs MI hacks.  */
};

/* Prototypes for local functions */

extern void _initialize_mi_out (void);
static void field_separator (struct ui_out *uiout);
static void mi_open (struct ui_out *uiout, const char *name,
		     enum ui_out_type type);
static void mi_close (struct ui_out *uiout, enum ui_out_type type);

/* Mark beginning of a table.  */

void
mi_table_begin (struct ui_out *uiout,
		int nr_cols,
		int nr_rows,
		const char *tblid)
{
  mi_open (uiout, tblid, ui_out_type_tuple);
  mi_field_int (uiout, -1, -1, ui_left, "nr_rows", nr_rows);
  mi_field_int (uiout, -1, -1, ui_left, "nr_cols", nr_cols);
  mi_open (uiout, "hdr", ui_out_type_list);
}

/* Mark beginning of a table body.  */

void
mi_table_body (struct ui_out *uiout)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;
  /* close the table header line if there were any headers */
  mi_close (uiout, ui_out_type_list);
  mi_open (uiout, "body", ui_out_type_list);
}

/* Mark end of a table.  */

void
mi_table_end (struct ui_out *uiout)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  data->suppress_output = 0;
  mi_close (uiout, ui_out_type_list); /* body */
  mi_close (uiout, ui_out_type_tuple);
}

/* Specify table header.  */

void
mi_table_header (struct ui_out *uiout, int width, enum ui_align alignment,
		 const char *col_name, const char *colhdr)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  mi_open (uiout, NULL, ui_out_type_tuple);
  mi_field_int (uiout, 0, 0, ui_center, "width", width);
  mi_field_int (uiout, 0, 0, ui_center, "alignment", alignment);
  mi_field_string (uiout, 0, 0, ui_center, "col_name", col_name);
  mi_field_string (uiout, 0, width, alignment, "colhdr", colhdr);
  mi_close (uiout, ui_out_type_tuple);
}

/* Mark beginning of a list.  */

void
mi_begin (struct ui_out *uiout, enum ui_out_type type, int level,
	  const char *id)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  mi_open (uiout, id, type);
}

/* Mark end of a list.  */

void
mi_end (struct ui_out *uiout, enum ui_out_type type, int level)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  mi_close (uiout, type);
}

/* Output an int field.  */

static void
mi_field_int (struct ui_out *uiout, int fldno, int width,
              enum ui_align alignment, const char *fldname, int value)
{
  char buffer[20];	/* FIXME: how many chars long a %d can become? */
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  if (data->suppress_output)
    return;

  xsnprintf (buffer, sizeof (buffer), "%d", value);
  mi_field_string (uiout, fldno, width, alignment, fldname, buffer);
}

/* Used to omit a field.  */

void
mi_field_skip (struct ui_out *uiout, int fldno, int width,
               enum ui_align alignment, const char *fldname)
{
}

/* Other specific mi_field_* end up here so alignment and field
   separators are both handled by mi_field_string. */

void
mi_field_string (struct ui_out *uiout, int fldno, int width,
		 enum ui_align align, const char *fldname, const char *string)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream;

  if (data->suppress_output)
    return;

  stream = VEC_last (ui_filep, data->streams);
  field_separator (uiout);
  if (fldname)
    fprintf_unfiltered (stream, "%s=", fldname);
  fprintf_unfiltered (stream, "\"");
  if (string)
    fputstr_unfiltered (string, '"', stream);
  fprintf_unfiltered (stream, "\"");
}

/* This is the only field function that does not align.  */

void
mi_field_fmt (struct ui_out *uiout, int fldno, int width,
	      enum ui_align align, const char *fldname,
	      const char *format, va_list args)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream;

  if (data->suppress_output)
    return;

  stream = VEC_last (ui_filep, data->streams);
  field_separator (uiout);
  if (fldname)
    fprintf_unfiltered (stream, "%s=\"", fldname);
  else
    fputs_unfiltered ("\"", stream);
  vfprintf_unfiltered (stream, format, args);
  fputs_unfiltered ("\"", stream);
}

void
mi_spaces (struct ui_out *uiout, int numspaces)
{
}

void
mi_text (struct ui_out *uiout, const char *string)
{
}

void
mi_message (struct ui_out *uiout, int verbosity,
	    const char *format, va_list args)
{
}

void
mi_wrap_hint (struct ui_out *uiout, char *identstring)
{
  wrap_here (identstring);
}

void
mi_flush (struct ui_out *uiout)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  gdb_flush (stream);
}

int
mi_redirect (struct ui_out *uiout, struct ui_file *outstream)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  if (outstream != NULL)
    VEC_safe_push (ui_filep, data->streams, outstream);
  else
    VEC_pop (ui_filep, data->streams);

  return 0;
}

/* local functions */

/* access to ui_out format private members */

static void
field_separator (struct ui_out *uiout)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  if (data->suppress_field_separator)
    data->suppress_field_separator = 0;
  else
    fputc_unfiltered (',', stream);
}

static void
mi_open (struct ui_out *uiout, const char *name, enum ui_out_type type)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  field_separator (uiout);
  data->suppress_field_separator = 1;
  if (name)
    fprintf_unfiltered (stream, "%s=", name);
  switch (type)
    {
    case ui_out_type_tuple:
      fputc_unfiltered ('{', stream);
      break;
    case ui_out_type_list:
      fputc_unfiltered ('[', stream);
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
}

static void
mi_close (struct ui_out *uiout, enum ui_out_type type)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  switch (type)
    {
    case ui_out_type_tuple:
      fputc_unfiltered ('}', stream);
      break;
    case ui_out_type_list:
      fputc_unfiltered (']', stream);
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
  data->suppress_field_separator = 0;
}

/* Add a string to the buffer.  */

void
mi_out_buffered (struct ui_out *uiout, char *string)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  fprintf_unfiltered (stream, "%s", string);
}

/* Clear the buffer.  */

void
mi_out_rewind (struct ui_out *uiout)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *stream = VEC_last (ui_filep, data->streams);

  ui_file_rewind (stream);
}

/* Dump the buffer onto the specified stream.  */

void
mi_out_put (struct ui_out *uiout, struct ui_file *stream)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);
  struct ui_file *outstream = VEC_last (ui_filep, data->streams);

  ui_file_put (outstream, ui_file_write_for_put, stream);
  ui_file_rewind (outstream);
}

/* Return the current MI version.  */

int
mi_version (struct ui_out *uiout)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (uiout);

  return data->mi_version;
}

/* Constructor for an `mi_out_data' object.  */

static void
mi_out_data_ctor (mi_out_data *self, int mi_version, struct ui_file *stream)
{
  gdb_assert (stream != NULL);

  self->streams = NULL;
  VEC_safe_push (ui_filep, self->streams, stream);

  self->suppress_field_separator = 0;
  self->suppress_output = 0;
  self->mi_version = mi_version;
}

/* The destructor.  */

static void
mi_out_data_dtor (struct ui_out *ui_out)
{
  mi_out_data *data = (mi_out_data *) ui_out_data (ui_out);

  VEC_free (ui_filep, data->streams);
  xfree (data);
}

/* Initialize private members at startup.  */

struct ui_out *
mi_out_new (int mi_version)
{
  int flags = 0;
  mi_out_data *data = XNEW (mi_out_data);
  struct ui_file *stream = mem_fileopen ();

  mi_out_data_ctor (data, mi_version, stream);
  return ui_out_new (&mi_ui_out_impl, data, flags);
}
