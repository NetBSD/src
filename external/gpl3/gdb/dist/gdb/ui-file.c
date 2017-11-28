/* UI_FILE - a generic STDIO like output stream.

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

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

/* Implement the ``struct ui_file'' object.  */

#include "defs.h"
#include "ui-file.h"
#include "gdb_obstack.h"
#include "gdb_select.h"
#include "filestuff.h"

null_file null_stream;

ui_file::ui_file ()
{}

ui_file::~ui_file ()
{}

void
ui_file::printf (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_unfiltered (this, format, args);
  va_end (args);
}

void
ui_file::putstr (const char *str, int quoter)
{
  fputstr_unfiltered (str, quoter, this);
}

void
ui_file::putstrn (const char *str, int n, int quoter)
{
  fputstrn_unfiltered (str, n, quoter, this);
}

int
ui_file::putc (int c)
{
  return fputc_unfiltered (c, this);
}

void
ui_file::vprintf (const char *format, va_list args)
{
  vfprintf_unfiltered (this, format, args);
}



void
null_file::write (const char *buf, long sizeof_buf)
{
  /* Discard the request.  */
}

void
null_file::puts (const char *)
{
  /* Discard the request.  */
}

void
null_file::write_async_safe (const char *buf, long sizeof_buf)
{
  /* Discard the request.  */
}



void
gdb_flush (struct ui_file *file)
{
  file->flush ();
}

int
ui_file_isatty (struct ui_file *file)
{
  return file->isatty ();
}

void
ui_file_write (struct ui_file *file,
		const char *buf,
		long length_buf)
{
  file->write (buf, length_buf);
}

void
ui_file_write_async_safe (struct ui_file *file,
			  const char *buf,
			  long length_buf)
{
  file->write_async_safe (buf, length_buf);
}

long
ui_file_read (struct ui_file *file, char *buf, long length_buf)
{
  return file->read (buf, length_buf);
}

void
fputs_unfiltered (const char *buf, struct ui_file *file)
{
  file->puts (buf);
}



string_file::~string_file ()
{}

void
string_file::write (const char *buf, long length_buf)
{
  m_string.append (buf, length_buf);
}



stdio_file::stdio_file (FILE *file, bool close_p)
{
  set_stream (file);
  m_close_p = close_p;
}

stdio_file::stdio_file ()
  : m_file (NULL),
    m_fd (-1),
    m_close_p (false)
{}

stdio_file::~stdio_file ()
{
  if (m_close_p)
    fclose (m_file);
}

void
stdio_file::set_stream (FILE *file)
{
  m_file = file;
  m_fd = fileno (file);
}

bool
stdio_file::open (const char *name, const char *mode)
{
  /* Close the previous stream, if we own it.  */
  if (m_close_p)
    {
      fclose (m_file);
      m_close_p = false;
    }

  FILE *f = gdb_fopen_cloexec (name, mode);

  if (f == NULL)
    return false;

  set_stream (f);
  m_close_p = true;

  return true;
}

void
stdio_file::flush ()
{
  fflush (m_file);
}

long
stdio_file::read (char *buf, long length_buf)
{
  /* Wait until at least one byte of data is available, or we get
     interrupted with Control-C.  */
  {
    fd_set readfds;

    FD_ZERO (&readfds);
    FD_SET (m_fd, &readfds);
    if (interruptible_select (m_fd + 1, &readfds, NULL, NULL, NULL) == -1)
      return -1;
  }

  return ::read (m_fd, buf, length_buf);
}

void
stdio_file::write (const char *buf, long length_buf)
{
  /* Calling error crashes when we are called from the exception framework.  */
  if (fwrite (buf, length_buf, 1, m_file))
    {
      /* Nothing.  */
    }
}

void
stdio_file::write_async_safe (const char *buf, long length_buf)
{
  /* This is written the way it is to avoid a warning from gcc about not using the
     result of write (since it can be declared with attribute warn_unused_result).
     Alas casting to void doesn't work for this.  */
  if (::write (m_fd, buf, length_buf))
    {
      /* Nothing.  */
    }
}

void
stdio_file::puts (const char *linebuffer)
{
  /* Calling error crashes when we are called from the exception framework.  */
  if (fputs (linebuffer, m_file))
    {
      /* Nothing.  */
    }
}

bool
stdio_file::isatty ()
{
  return ::isatty (m_fd);
}



/* This is the implementation of ui_file method 'write' for stderr.
   gdb_stdout is flushed before writing to gdb_stderr.  */

void
stderr_file::write (const char *buf, long length_buf)
{
  gdb_flush (gdb_stdout);
  stdio_file::write (buf, length_buf);
}

/* This is the implementation of ui_file method 'puts' for stderr.
   gdb_stdout is flushed before writing to gdb_stderr.  */

void
stderr_file::puts (const char *linebuffer)
{
  gdb_flush (gdb_stdout);
  stdio_file::puts (linebuffer);
}

stderr_file::stderr_file (FILE *stream)
  : stdio_file (stream)
{}



tee_file::tee_file (ui_file *one, bool close_one,
		    ui_file *two, bool close_two)
  : m_one (one),
    m_two (two),
    m_close_one (close_one),
    m_close_two (close_two)
{}

tee_file::~tee_file ()
{
  if (m_close_one)
    delete m_one;
  if (m_close_two)
    delete m_two;
}

void
tee_file::flush ()
{
  m_one->flush ();
  m_two->flush ();
}

void
tee_file::write (const char *buf, long length_buf)
{
  m_one->write (buf, length_buf);
  m_two->write (buf, length_buf);
}

void
tee_file::write_async_safe (const char *buf, long length_buf)
{
  m_one->write_async_safe (buf, length_buf);
  m_two->write_async_safe (buf, length_buf);
}

void
tee_file::puts (const char *linebuffer)
{
  m_one->puts (linebuffer);
  m_two->puts (linebuffer);
}

bool
tee_file::isatty ()
{
  return m_one->isatty ();
}
