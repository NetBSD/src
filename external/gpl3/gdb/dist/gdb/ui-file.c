/* UI_FILE - a generic STDIO like output stream.

   Copyright (C) 1999-2020 Free Software Foundation, Inc.

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
#include "gdbsupport/gdb_select.h"
#include "gdbsupport/filestuff.h"
#include "cli/cli-style.h"

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
  fputstrn_unfiltered (str, n, quoter, fputc_unfiltered, this);
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



/* true if the gdb terminal supports styling, and styling is enabled.  */

static bool
term_cli_styling ()
{
  if (!cli_styling)
    return false;

  const char *term = getenv ("TERM");
  /* Windows doesn't by default define $TERM, but can support styles
     regardless.  */
#ifndef _WIN32
  if (term == nullptr || !strcmp (term, "dumb"))
    return false;
#else
  /* But if they do define $TERM, let us behave the same as on Posix
     platforms, for the benefit of programs which invoke GDB as their
     back-end.  */
  if (term && !strcmp (term, "dumb"))
    return false;
#endif
  return true;
}



string_file::~string_file ()
{}

void
string_file::write (const char *buf, long length_buf)
{
  m_string.append (buf, length_buf);
}

/* See ui-file.h.  */

bool
string_file::term_out ()
{
  return m_term_out;
}

/* See ui-file.h.  */

bool
string_file::can_emit_style_escape ()
{
  return m_term_out && term_cli_styling ();
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

  gdb_file_up f = gdb_fopen_cloexec (name, mode);

  if (f == NULL)
    return false;

  set_stream (f.release ());
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
  /* This host-dependent function (with implementations in
     posix-hdep.c and mingw-hdep.c) is given the opportunity to
     process the output first in host-dependent way.  If it does, it
     should return non-zero, to avoid calling fputs below.  */
  if (gdb_console_fputs (linebuffer, m_file))
    return;
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

/* See ui-file.h.  */

bool
stdio_file::can_emit_style_escape ()
{
  return ((this == gdb_stdout || this == gdb_stderr)
	  && this->isatty ()
	  && term_cli_styling ());
}



/* This is the implementation of ui_file method 'write' for stderr.
   gdb_stdout is flushed before writing to gdb_stderr.  */

void
stderr_file::write (const char *buf, long length_buf)
{
  gdb_stdout->flush ();
  stdio_file::write (buf, length_buf);
}

/* This is the implementation of ui_file method 'puts' for stderr.
   gdb_stdout is flushed before writing to gdb_stderr.  */

void
stderr_file::puts (const char *linebuffer)
{
  gdb_stdout->flush ();
  stdio_file::puts (linebuffer);
}

stderr_file::stderr_file (FILE *stream)
  : stdio_file (stream)
{}



tee_file::tee_file (ui_file *one, ui_file_up &&two)
  : m_one (one),
    m_two (std::move (two))
{}

tee_file::~tee_file ()
{
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

/* See ui-file.h.  */

bool
tee_file::term_out ()
{
  return m_one->term_out ();
}

/* See ui-file.h.  */

bool
tee_file::can_emit_style_escape ()
{
  return ((this == gdb_stdout || this == gdb_stderr)
	  && m_one->term_out ()
	  && term_cli_styling ());
}

/* See ui-file.h.  */

void
no_terminal_escape_file::write (const char *buf, long length_buf)
{
  std::string copy (buf, length_buf);
  this->puts (copy.c_str ());
}

/* See ui-file.h.  */

void
no_terminal_escape_file::puts (const char *buf)
{
  while (*buf != '\0')
    {
      const char *esc = strchr (buf, '\033');
      if (esc == nullptr)
	break;

      int n_read = 0;
      if (!skip_ansi_escape (esc, &n_read))
	++esc;

      this->stdio_file::write (buf, esc - buf);
      buf = esc + n_read;
    }

  if (*buf != '\0')
    this->stdio_file::write (buf, strlen (buf));
}
