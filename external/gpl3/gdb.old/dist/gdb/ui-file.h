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

#ifndef UI_FILE_H
#define UI_FILE_H

#include <string>

/* The abstract ui_file base class.  */

class ui_file
{
public:
  ui_file ();
  virtual ~ui_file () = 0;

  /* Public non-virtual API.  */

  void printf (const char *, ...) ATTRIBUTE_PRINTF (2, 3);

  /* Print a string whose delimiter is QUOTER.  Note that these
     routines should only be called for printing things which are
     independent of the language of the program being debugged.  */
  void putstr (const char *str, int quoter);

  void putstrn (const char *str, int n, int quoter);

  int putc (int c);

  void vprintf (const char *, va_list) ATTRIBUTE_PRINTF (2, 0);

  /* Methods below are both public, and overridable by ui_file
     subclasses.  */

  virtual void write (const char *buf, long length_buf) = 0;

  /* This version of "write" is safe for use in signal handlers.  It's
     not guaranteed that all existing output will have been flushed
     first.  Implementations are also free to ignore some or all of
     the request.  puts_async is not provided as the async versions
     are rarely used, no point in having both for a rarely used
     interface.  */
  virtual void write_async_safe (const char *buf, long length_buf)
  { gdb_assert_not_reached ("write_async_safe"); }

  /* Some ui_files override this to provide a efficient implementation
     that avoids a strlen.  */
  virtual void puts (const char *str)
  { this->write (str, strlen (str)); }

  virtual long read (char *buf, long length_buf)
  { gdb_assert_not_reached ("can't read from this file type"); }

  virtual bool isatty ()
  { return false; }

  virtual void flush ()
  {}
};

typedef std::unique_ptr<ui_file> ui_file_up;

/* A ui_file that writes to nowhere.  */

class null_file : public ui_file
{
public:
  void write (const char *buf, long length_buf) override;
  void write_async_safe (const char *buf, long sizeof_buf) override;
  void puts (const char *str) override;
};

/* A preallocated null_file stream.  */
extern null_file null_stream;

extern void gdb_flush (ui_file *);

extern int ui_file_isatty (struct ui_file *);

extern void ui_file_write (struct ui_file *file, const char *buf,
			   long length_buf);

extern void ui_file_write_async_safe (struct ui_file *file, const char *buf,
				      long length_buf);

extern long ui_file_read (struct ui_file *file, char *buf, long length_buf);

/* A std::string-based ui_file.  Can be used as a scratch buffer for
   collecting output.  */

class string_file : public ui_file
{
public:
  string_file () {}
  ~string_file () override;

  /* Override ui_file methods.  */

  void write (const char *buf, long length_buf) override;

  long read (char *buf, long length_buf) override
  { gdb_assert_not_reached ("a string_file is not readable"); }

  /* string_file-specific public API.  */

  /* Accesses the std::string containing the entire output collected
     so far.

     Returns a non-const reference so that it's easy to move the
     string contents out of the string_file.  E.g.:

      string_file buf;
      buf.printf (....);
      buf.printf (....);
      return std::move (buf.string ());
  */
  std::string &string () { return m_string; }

  /* Provide a few convenience methods with the same API as the
     underlying std::string.  */
  const char *data () const { return m_string.data (); }
  const char *c_str () const { return m_string.c_str (); }
  size_t size () const { return m_string.size (); }
  bool empty () const { return m_string.empty (); }
  void clear () { return m_string.clear (); }

private:
  /* The internal buffer.  */
  std::string m_string;
};

/* A ui_file implementation that maps directly onto <stdio.h>'s FILE.
   A stdio_file can either own its underlying file, or not.  If it
   owns the file, then destroying the stdio_file closes the underlying
   file, otherwise it is left open.  */

class stdio_file : public ui_file
{
public:
  /* Create a ui_file from a previously opened FILE.  CLOSE_P
     indicates whether the underlying file should be closed when the
     stdio_file is destroyed.  */
  explicit stdio_file (FILE *file, bool close_p = false);

  /* Create an stdio_file that is not managing any file yet.  Call
     open to actually open something.  */
  stdio_file ();

  ~stdio_file () override;

  /* Open NAME in mode MODE, and own the resulting file.  Returns true
     on success, false otherwise.  If the stdio_file previously owned
     a file, it is closed.  */
  bool open (const char *name, const char *mode);

  void flush () override;

  void write (const char *buf, long length_buf) override;

  void write_async_safe (const char *buf, long length_buf) override;

  void puts (const char *) override;

  long read (char *buf, long length_buf) override;

  bool isatty () override;

private:
  /* Sets the internal stream to FILE, and saves the FILE's file
     descriptor in M_FD.  */
  void set_stream (FILE *file);

  /* The file.  */
  FILE *m_file;

  /* The associated file descriptor is extracted ahead of time for
     stdio_file::write_async_safe's benefit, in case fileno isn't
     async-safe.  */
  int m_fd;

  /* If true, M_FILE is closed on destruction.  */
  bool m_close_p;
};

typedef std::unique_ptr<stdio_file> stdio_file_up;

/* Like stdio_file, but specifically for stderr.

   This exists because there is no real line-buffering on Windows, see
   <http://msdn.microsoft.com/en-us/library/86cebhfs%28v=vs.71%29.aspx>
   so the stdout is either fully-buffered or non-buffered.  We can't
   make stdout non-buffered, because of two concerns:

    1. Non-buffering hurts performance.
    2. Non-buffering may change GDB's behavior when it is interacting
       with a front-end, such as Emacs.

   We leave stdout as fully buffered, but flush it first when
   something is written to stderr.

   Note that the 'write_async_safe' method is not overridden, because
   there's no way to flush a stream in an async-safe manner.
   Fortunately, it doesn't really matter, because:

    1. That method is only used for printing internal debug output
       from signal handlers.

    2. Windows hosts don't have a concept of async-safeness.  Signal
       handlers run in a separate thread, so they can call the regular
       non-async-safe output routines freely.
*/
class stderr_file : public stdio_file
{
public:
  explicit stderr_file (FILE *stream);

  /* Override the output routines to flush gdb_stdout before deferring
     to stdio_file for the actual outputting.  */
  void write (const char *buf, long length_buf) override;
  void puts (const char *linebuffer) override;
};

/* A ui_file implementation that maps onto two ui-file objects.  */

class tee_file : public ui_file
{
public:
  /* Create a file which writes to both ONE and TWO.  CLOSE_ONE and
     CLOSE_TWO indicate whether the original files should be closed
     when the new file is closed.  */
  tee_file (ui_file *one, bool close_one,
	    ui_file *two, bool close_two);
  ~tee_file () override;

  void write (const char *buf, long length_buf) override;
  void write_async_safe (const char *buf, long length_buf) override;
  void puts (const char *) override;

  bool isatty () override;
  void flush () override;

private:
  /* The two underlying ui_files, and whether they should each be
     closed on destruction.  */
  ui_file *m_one, *m_two;
  bool m_close_one, m_close_two;
};

#endif
