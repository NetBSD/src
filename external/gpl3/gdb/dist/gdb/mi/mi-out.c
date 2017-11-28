/* MI Command Set - output generating routines.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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
#include <vector>

/* Mark beginning of a table.  */

void
mi_ui_out::do_table_begin (int nr_cols, int nr_rows,
			   const char *tblid)
{
  open (tblid, ui_out_type_tuple);
  do_field_int (-1, -1, ui_left, "nr_rows", nr_rows);
  do_field_int (-1, -1, ui_left, "nr_cols", nr_cols);
  open ("hdr", ui_out_type_list);
}

/* Mark beginning of a table body.  */

void
mi_ui_out::do_table_body ()
{
  /* close the table header line if there were any headers */
  close (ui_out_type_list);
  open ("body", ui_out_type_list);
}

/* Mark end of a table.  */

void
mi_ui_out::do_table_end ()
{
  close (ui_out_type_list); /* body */
  close (ui_out_type_tuple);
}

/* Specify table header.  */

void
mi_ui_out::do_table_header (int width, ui_align alignment,
			    const std::string &col_name,
			    const std::string &col_hdr)
{
  open (NULL, ui_out_type_tuple);
  do_field_int (0, 0, ui_center, "width", width);
  do_field_int (0, 0, ui_center, "alignment", alignment);
  do_field_string (0, 0, ui_center, "col_name", col_name.c_str ());
  do_field_string (0, width, alignment, "colhdr", col_hdr.c_str ());
  close (ui_out_type_tuple);
}

/* Mark beginning of a list.  */

void
mi_ui_out::do_begin (ui_out_type type, const char *id)
{
  open (id, type);
}

/* Mark end of a list.  */

void
mi_ui_out::do_end (ui_out_type type)
{
  close (type);
}

/* Output an int field.  */

void
mi_ui_out::do_field_int (int fldno, int width, ui_align alignment,
			 const char *fldname, int value)
{
  char buffer[20];	/* FIXME: how many chars long a %d can become? */

  xsnprintf (buffer, sizeof (buffer), "%d", value);
  do_field_string (fldno, width, alignment, fldname, buffer);
}

/* Used to omit a field.  */

void
mi_ui_out::do_field_skip (int fldno, int width, ui_align alignment,
			  const char *fldname)
{
}

/* Other specific mi_field_* end up here so alignment and field
   separators are both handled by mi_field_string. */

void
mi_ui_out::do_field_string (int fldno, int width, ui_align align,
			    const char *fldname, const char *string)
{
  ui_file *stream = m_streams.back ();
  field_separator ();

  if (fldname)
    fprintf_unfiltered (stream, "%s=", fldname);
  fprintf_unfiltered (stream, "\"");
  if (string)
    fputstr_unfiltered (string, '"', stream);
  fprintf_unfiltered (stream, "\"");
}

/* This is the only field function that does not align.  */

void
mi_ui_out::do_field_fmt (int fldno, int width, ui_align align,
			 const char *fldname, const char *format,
			 va_list args)
{
  ui_file *stream = m_streams.back ();
  field_separator ();

  if (fldname)
    fprintf_unfiltered (stream, "%s=\"", fldname);
  else
    fputs_unfiltered ("\"", stream);
  vfprintf_unfiltered (stream, format, args);
  fputs_unfiltered ("\"", stream);
}

void
mi_ui_out::do_spaces (int numspaces)
{
}

void
mi_ui_out::do_text (const char *string)
{
}

void
mi_ui_out::do_message (const char *format, va_list args)
{
}

void
mi_ui_out::do_wrap_hint (const char *identstring)
{
  wrap_here (identstring);
}

void
mi_ui_out::do_flush ()
{

  gdb_flush (m_streams.back ());
}

void
mi_ui_out::do_redirect (ui_file *outstream)
{
  if (outstream != NULL)
    m_streams.push_back (outstream);
  else
    m_streams.pop_back ();
}

void
mi_ui_out::field_separator ()
{
  if (m_suppress_field_separator)
    m_suppress_field_separator = false;
  else
    fputc_unfiltered (',', m_streams.back ());
}

void
mi_ui_out::open (const char *name, ui_out_type type)
{
  ui_file *stream = m_streams.back ();

  field_separator ();
  m_suppress_field_separator = true;

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

void
mi_ui_out::close (ui_out_type type)
{
  ui_file *stream = m_streams.back ();

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

  m_suppress_field_separator = false;
}

string_file *
mi_ui_out::main_stream ()
{
  gdb_assert (m_streams.size () == 1);

  return (string_file *) m_streams.back ();
}

/* Clear the buffer.  */

void
mi_ui_out::rewind ()
{
  main_stream ()->clear ();
}

/* Dump the buffer onto the specified stream.  */

void
mi_ui_out::put (ui_file *where)
{
  string_file *mi_stream = main_stream ();

  where->write (mi_stream->data (), mi_stream->size ());
  mi_stream->clear ();
}

/* Return the current MI version.  */

int
mi_ui_out::version ()
{
  return m_mi_version;
}

/* Constructor for an `mi_out_data' object.  */

mi_ui_out::mi_ui_out (int mi_version)
: m_suppress_field_separator (false),
  m_suppress_output (false),
  m_mi_version (mi_version)
{
  string_file *stream = new string_file ();
  m_streams.push_back (stream);
}

mi_ui_out::~mi_ui_out ()
{
}

/* Initialize private members at startup.  */

mi_ui_out *
mi_out_new (int mi_version)
{
  return new mi_ui_out (mi_version);
}

/* Helper function to return the given UIOUT as an mi_ui_out.  It is an error
   to call this function with an ui_out that is not an MI.  */

static mi_ui_out *
as_mi_ui_out (ui_out *uiout)
{
  mi_ui_out *mi_uiout = dynamic_cast<mi_ui_out *> (uiout);

  gdb_assert (mi_uiout != NULL);

  return mi_uiout;
}

int
mi_version (ui_out *uiout)
{
  return as_mi_ui_out (uiout)->version ();
}

void
mi_out_put (ui_out *uiout, struct ui_file *stream)
{
  return as_mi_ui_out (uiout)->put (stream);
}

void
mi_out_rewind (ui_out *uiout)
{
  return as_mi_ui_out (uiout)->rewind ();
}
