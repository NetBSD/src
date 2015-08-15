/* UI_FILE - a generic STDIO like output stream.

   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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
#include <string.h>
#include "gdb_select.h"
#include "filestuff.h"

#include <errno.h>

static ui_file_isatty_ftype null_file_isatty;
static ui_file_write_ftype null_file_write;
static ui_file_write_ftype null_file_write_async_safe;
static ui_file_fputs_ftype null_file_fputs;
static ui_file_read_ftype null_file_read;
static ui_file_flush_ftype null_file_flush;
static ui_file_delete_ftype null_file_delete;
static ui_file_rewind_ftype null_file_rewind;
static ui_file_put_ftype null_file_put;
static ui_file_fseek_ftype null_file_fseek;

struct ui_file
  {
    int *magic;
    ui_file_flush_ftype *to_flush;
    ui_file_write_ftype *to_write;
    ui_file_write_async_safe_ftype *to_write_async_safe;
    ui_file_fputs_ftype *to_fputs;
    ui_file_read_ftype *to_read;
    ui_file_delete_ftype *to_delete;
    ui_file_isatty_ftype *to_isatty;
    ui_file_rewind_ftype *to_rewind;
    ui_file_put_ftype *to_put;
    ui_file_fseek_ftype *to_fseek;
    void *to_data;
  };
int ui_file_magic;

struct ui_file *
ui_file_new (void)
{
  struct ui_file *file = xmalloc (sizeof (struct ui_file));

  file->magic = &ui_file_magic;
  set_ui_file_data (file, NULL, null_file_delete);
  set_ui_file_flush (file, null_file_flush);
  set_ui_file_write (file, null_file_write);
  set_ui_file_write_async_safe (file, null_file_write_async_safe);
  set_ui_file_fputs (file, null_file_fputs);
  set_ui_file_read (file, null_file_read);
  set_ui_file_isatty (file, null_file_isatty);
  set_ui_file_rewind (file, null_file_rewind);
  set_ui_file_put (file, null_file_put);
  set_ui_file_fseek (file, null_file_fseek);
  return file;
}

void
ui_file_delete (struct ui_file *file)
{
  file->to_delete (file);
  xfree (file);
}

static int
null_file_isatty (struct ui_file *file)
{
  return 0;
}

static void
null_file_rewind (struct ui_file *file)
{
  return;
}

static void
null_file_put (struct ui_file *file,
	       ui_file_put_method_ftype *write,
	       void *dest)
{
  return;
}

static void
null_file_flush (struct ui_file *file)
{
  return;
}

static void
null_file_write (struct ui_file *file,
		 const char *buf,
		 long sizeof_buf)
{
  if (file->to_fputs == null_file_fputs)
    /* Both the write and fputs methods are null.  Discard the
       request.  */
    return;
  else
    {
      /* The fputs method isn't null, slowly pass the write request
         onto that.  FYI, this isn't as bad as it may look - the
         current (as of 1999-11-07) printf_* function calls fputc and
         fputc does exactly the below.  By having a write function it
         is possible to clean up that code.  */
      int i;
      char b[2];

      b[1] = '\0';
      for (i = 0; i < sizeof_buf; i++)
	{
	  b[0] = buf[i];
	  file->to_fputs (b, file);
	}
      return;
    }
}

static long
null_file_read (struct ui_file *file,
		char *buf,
		long sizeof_buf)
{
  errno = EBADF;
  return 0;
}

static void
null_file_fputs (const char *buf, struct ui_file *file)
{
  if (file->to_write == null_file_write)
    /* Both the write and fputs methods are null.  Discard the
       request.  */
    return;
  else
    {
      /* The write method was implemented, use that.  */
      file->to_write (file, buf, strlen (buf));
    }
}

static void
null_file_write_async_safe (struct ui_file *file,
			    const char *buf,
			    long sizeof_buf)
{
  return;
}

static void
null_file_delete (struct ui_file *file)
{
  return;
}

static int
null_file_fseek (struct ui_file *stream, long offset, int whence)
{
  errno = EBADF;

  return -1;
}

void *
ui_file_data (struct ui_file *file)
{
  if (file->magic != &ui_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("ui_file_data: bad magic number"));
  return file->to_data;
}

void
gdb_flush (struct ui_file *file)
{
  file->to_flush (file);
}

int
ui_file_isatty (struct ui_file *file)
{
  return file->to_isatty (file);
}

void
ui_file_rewind (struct ui_file *file)
{
  file->to_rewind (file);
}

void
ui_file_put (struct ui_file *file,
	      ui_file_put_method_ftype *write,
	      void *dest)
{
  file->to_put (file, write, dest);
}

void
ui_file_write (struct ui_file *file,
		const char *buf,
		long length_buf)
{
  file->to_write (file, buf, length_buf);
}

void
ui_file_write_async_safe (struct ui_file *file,
			  const char *buf,
			  long length_buf)
{
  file->to_write_async_safe (file, buf, length_buf);
}

long
ui_file_read (struct ui_file *file, char *buf, long length_buf)
{
  return file->to_read (file, buf, length_buf); 
}

int
ui_file_fseek (struct ui_file *file, long offset, int whence)
{
  return file->to_fseek (file, offset, whence);
}

void
fputs_unfiltered (const char *buf, struct ui_file *file)
{
  file->to_fputs (buf, file);
}

void
set_ui_file_flush (struct ui_file *file, ui_file_flush_ftype *flush_ptr)
{
  file->to_flush = flush_ptr;
}

void
set_ui_file_isatty (struct ui_file *file, ui_file_isatty_ftype *isatty_ptr)
{
  file->to_isatty = isatty_ptr;
}

void
set_ui_file_rewind (struct ui_file *file, ui_file_rewind_ftype *rewind_ptr)
{
  file->to_rewind = rewind_ptr;
}

void
set_ui_file_put (struct ui_file *file, ui_file_put_ftype *put_ptr)
{
  file->to_put = put_ptr;
}

void
set_ui_file_write (struct ui_file *file,
		    ui_file_write_ftype *write_ptr)
{
  file->to_write = write_ptr;
}

void
set_ui_file_write_async_safe (struct ui_file *file,
			      ui_file_write_async_safe_ftype *write_async_safe_ptr)
{
  file->to_write_async_safe = write_async_safe_ptr;
}

void
set_ui_file_read (struct ui_file *file, ui_file_read_ftype *read_ptr)
{
  file->to_read = read_ptr;
}

void
set_ui_file_fputs (struct ui_file *file, ui_file_fputs_ftype *fputs_ptr)
{
  file->to_fputs = fputs_ptr;
}

void
set_ui_file_fseek (struct ui_file *file, ui_file_fseek_ftype *fseek_ptr)
{
  file->to_fseek = fseek_ptr;
}

void
set_ui_file_data (struct ui_file *file, void *data,
		  ui_file_delete_ftype *delete_ptr)
{
  file->to_data = data;
  file->to_delete = delete_ptr;
}

/* ui_file utility function for converting a ``struct ui_file'' into
   a memory buffer.  */

struct accumulated_ui_file
{
  char *buffer;
  long length;
};

static void
do_ui_file_xstrdup (void *context, const char *buffer, long length)
{
  struct accumulated_ui_file *acc = context;

  if (acc->buffer == NULL)
    acc->buffer = xmalloc (length + 1);
  else
    acc->buffer = xrealloc (acc->buffer, acc->length + length + 1);
  memcpy (acc->buffer + acc->length, buffer, length);
  acc->length += length;
  acc->buffer[acc->length] = '\0';
}

char *
ui_file_xstrdup (struct ui_file *file, long *length)
{
  struct accumulated_ui_file acc;

  acc.buffer = NULL;
  acc.length = 0;
  ui_file_put (file, do_ui_file_xstrdup, &acc);
  if (acc.buffer == NULL)
    acc.buffer = xstrdup ("");
  if (length != NULL)
    *length = acc.length;
  return acc.buffer;
}

static void
do_ui_file_obsavestring (void *context, const char *buffer, long length)
{
  struct obstack *obstack = (struct obstack *) context;

  obstack_grow (obstack, buffer, length);
}

char *
ui_file_obsavestring (struct ui_file *file, struct obstack *obstack,
		      long *length)
{
  ui_file_put (file, do_ui_file_obsavestring, obstack);
  *length = obstack_object_size (obstack);
  obstack_1grow (obstack, '\0');
  return obstack_finish (obstack);
}

/* A pure memory based ``struct ui_file'' that can be used an output
   buffer.  The buffers accumulated contents are available via
   ui_file_put().  */

struct mem_file
  {
    int *magic;
    char *buffer;
    int sizeof_buffer;
    int length_buffer;
  };

static ui_file_rewind_ftype mem_file_rewind;
static ui_file_put_ftype mem_file_put;
static ui_file_write_ftype mem_file_write;
static ui_file_delete_ftype mem_file_delete;
static struct ui_file *mem_file_new (void);
static int mem_file_magic;

static struct ui_file *
mem_file_new (void)
{
  struct mem_file *stream = XMALLOC (struct mem_file);
  struct ui_file *file = ui_file_new ();

  set_ui_file_data (file, stream, mem_file_delete);
  set_ui_file_rewind (file, mem_file_rewind);
  set_ui_file_put (file, mem_file_put);
  set_ui_file_write (file, mem_file_write);
  stream->magic = &mem_file_magic;
  stream->buffer = NULL;
  stream->sizeof_buffer = 0;
  stream->length_buffer = 0;
  return file;
}

static void
mem_file_delete (struct ui_file *file)
{
  struct mem_file *stream = ui_file_data (file);

  if (stream->magic != &mem_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("mem_file_delete: bad magic number"));
  if (stream->buffer != NULL)
    xfree (stream->buffer);
  xfree (stream);
}

struct ui_file *
mem_fileopen (void)
{
  return mem_file_new ();
}

static void
mem_file_rewind (struct ui_file *file)
{
  struct mem_file *stream = ui_file_data (file);

  if (stream->magic != &mem_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("mem_file_rewind: bad magic number"));
  stream->length_buffer = 0;
}

static void
mem_file_put (struct ui_file *file,
	      ui_file_put_method_ftype *write,
	      void *dest)
{
  struct mem_file *stream = ui_file_data (file);

  if (stream->magic != &mem_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("mem_file_put: bad magic number"));
  if (stream->length_buffer > 0)
    write (dest, stream->buffer, stream->length_buffer);
}

void
mem_file_write (struct ui_file *file,
		const char *buffer,
		long length_buffer)
{
  struct mem_file *stream = ui_file_data (file);

  if (stream->magic != &mem_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("mem_file_write: bad magic number"));
  if (stream->buffer == NULL)
    {
      stream->length_buffer = length_buffer;
      stream->sizeof_buffer = length_buffer;
      stream->buffer = xmalloc (stream->sizeof_buffer);
      memcpy (stream->buffer, buffer, length_buffer);
    }
  else
    {
      int new_length = stream->length_buffer + length_buffer;

      if (new_length >= stream->sizeof_buffer)
	{
	  stream->sizeof_buffer = new_length;
	  stream->buffer = xrealloc (stream->buffer, stream->sizeof_buffer);
	}
      memcpy (stream->buffer + stream->length_buffer, buffer, length_buffer);
      stream->length_buffer = new_length;
    }
}

/* ``struct ui_file'' implementation that maps directly onto
   <stdio.h>'s FILE.  */

static ui_file_write_ftype stdio_file_write;
static ui_file_write_async_safe_ftype stdio_file_write_async_safe;
static ui_file_fputs_ftype stdio_file_fputs;
static ui_file_read_ftype stdio_file_read;
static ui_file_isatty_ftype stdio_file_isatty;
static ui_file_delete_ftype stdio_file_delete;
static struct ui_file *stdio_file_new (FILE *file, int close_p);
static ui_file_flush_ftype stdio_file_flush;
static ui_file_fseek_ftype stdio_file_fseek;

static int stdio_file_magic;

struct stdio_file
  {
    int *magic;
    FILE *file;
    /* The associated file descriptor is extracted ahead of time for
       stdio_file_write_async_safe's benefit, in case fileno isn't async-safe.  */
    int fd;
    int close_p;
  };

static struct ui_file *
stdio_file_new (FILE *file, int close_p)
{
  struct ui_file *ui_file = ui_file_new ();
  struct stdio_file *stdio = xmalloc (sizeof (struct stdio_file));

  stdio->magic = &stdio_file_magic;
  stdio->file = file;
  stdio->fd = fileno (file);
  stdio->close_p = close_p;
  set_ui_file_data (ui_file, stdio, stdio_file_delete);
  set_ui_file_flush (ui_file, stdio_file_flush);
  set_ui_file_write (ui_file, stdio_file_write);
  set_ui_file_write_async_safe (ui_file, stdio_file_write_async_safe);
  set_ui_file_fputs (ui_file, stdio_file_fputs);
  set_ui_file_read (ui_file, stdio_file_read);
  set_ui_file_isatty (ui_file, stdio_file_isatty);
  set_ui_file_fseek (ui_file, stdio_file_fseek);
  return ui_file;
}

static void
stdio_file_delete (struct ui_file *file)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_delete: bad magic number"));
  if (stdio->close_p)
    {
      fclose (stdio->file);
    }
  xfree (stdio);
}

static void
stdio_file_flush (struct ui_file *file)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_flush: bad magic number"));
  fflush (stdio->file);
}

static long
stdio_file_read (struct ui_file *file, char *buf, long length_buf)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_read: bad magic number"));

  /* For the benefit of Windows, call gdb_select before reading from
     the file.  Wait until at least one byte of data is available.
     Control-C can interrupt gdb_select, but not read.  */
  {
    fd_set readfds;
    FD_ZERO (&readfds);
    FD_SET (stdio->fd, &readfds);
    if (gdb_select (stdio->fd + 1, &readfds, NULL, NULL, NULL) == -1)
      return -1;
  }

  return read (stdio->fd, buf, length_buf);
}

static void
stdio_file_write (struct ui_file *file, const char *buf, long length_buf)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_write: bad magic number"));
  /* Calling error crashes when we are called from the exception framework.  */
  if (fwrite (buf, length_buf, 1, stdio->file))
    {
      /* Nothing.  */
    }
}

static void
stdio_file_write_async_safe (struct ui_file *file,
			     const char *buf, long length_buf)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    {
      /* gettext isn't necessarily async safe, so we can't use _("error message") here.
	 We could extract the correct translation ahead of time, but this is an extremely
	 rare event, and one of the other stdio_file_* routines will presumably catch
	 the problem anyway.  For now keep it simple and ignore the error here.  */
      return;
    }

  /* This is written the way it is to avoid a warning from gcc about not using the
     result of write (since it can be declared with attribute warn_unused_result).
     Alas casting to void doesn't work for this.  */
  if (write (stdio->fd, buf, length_buf))
    {
      /* Nothing.  */
    }
}

static void
stdio_file_fputs (const char *linebuffer, struct ui_file *file)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_fputs: bad magic number"));
  /* Calling error crashes when we are called from the exception framework.  */
  if (fputs (linebuffer, stdio->file))
    {
      /* Nothing.  */
    }
}

static int
stdio_file_isatty (struct ui_file *file)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_isatty: bad magic number"));
  return (isatty (stdio->fd));
}

static int
stdio_file_fseek (struct ui_file *file, long offset, int whence)
{
  struct stdio_file *stdio = ui_file_data (file);

  if (stdio->magic != &stdio_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("stdio_file_fseek: bad magic number"));

  return fseek (stdio->file, offset, whence);
}

#ifdef __MINGW32__
/* This is the implementation of ui_file method to_write for stderr.
   gdb_stdout is flushed before writing to gdb_stderr.  */

static void
stderr_file_write (struct ui_file *file, const char *buf, long length_buf)
{
  gdb_flush (gdb_stdout);
  stdio_file_write (file, buf, length_buf);
}

/* This is the implementation of ui_file method to_fputs for stderr.
   gdb_stdout is flushed before writing to gdb_stderr.  */

static void
stderr_file_fputs (const char *linebuffer, struct ui_file *file)
{
  gdb_flush (gdb_stdout);
  stdio_file_fputs (linebuffer, file);
}
#endif

struct ui_file *
stderr_fileopen (void)
{
  struct ui_file *ui_file = stdio_fileopen (stderr);

#ifdef __MINGW32__
  /* There is no real line-buffering on Windows, see
     http://msdn.microsoft.com/en-us/library/86cebhfs%28v=vs.71%29.aspx
     so the stdout is either fully-buffered or non-buffered.  We can't
     make stdout non-buffered, because of two concerns,
     1.  non-buffering hurts performance,
     2.  non-buffering may change GDB's behavior when it is interacting
     with front-end, such as Emacs.

     We decided to leave stdout as fully buffered, but flush it first
     when something is written to stderr.  */

  /* Method 'to_write_async_safe' is not overwritten, because there's
     no way to flush a stream in an async-safe manner.  Fortunately,
     it doesn't really matter, because:
     - that method is only used for printing internal debug output
       from signal handlers.
     - Windows hosts don't have a concept of async-safeness.  Signal
       handlers run in a separate thread, so they can call
       the regular non-async-safe output routines freely.  */
  set_ui_file_write (ui_file, stderr_file_write);
  set_ui_file_fputs (ui_file, stderr_file_fputs);
#endif

  return ui_file;
}

/* Like fdopen().  Create a ui_file from a previously opened FILE.  */

struct ui_file *
stdio_fileopen (FILE *file)
{
  return stdio_file_new (file, 0);
}

struct ui_file *
gdb_fopen (const char *name, const char *mode)
{
  FILE *f = gdb_fopen_cloexec (name, mode);

  if (f == NULL)
    return NULL;
  return stdio_file_new (f, 1);
}

/* ``struct ui_file'' implementation that maps onto two ui-file objects.  */

static ui_file_write_ftype tee_file_write;
static ui_file_fputs_ftype tee_file_fputs;
static ui_file_isatty_ftype tee_file_isatty;
static ui_file_delete_ftype tee_file_delete;
static ui_file_flush_ftype tee_file_flush;

static int tee_file_magic;

struct tee_file
  {
    int *magic;
    struct ui_file *one, *two;
    int close_one, close_two;
  };

struct ui_file *
tee_file_new (struct ui_file *one, int close_one,
	      struct ui_file *two, int close_two)
{
  struct ui_file *ui_file = ui_file_new ();
  struct tee_file *tee = xmalloc (sizeof (struct tee_file));

  tee->magic = &tee_file_magic;
  tee->one = one;
  tee->two = two;
  tee->close_one = close_one;
  tee->close_two = close_two;
  set_ui_file_data (ui_file, tee, tee_file_delete);
  set_ui_file_flush (ui_file, tee_file_flush);
  set_ui_file_write (ui_file, tee_file_write);
  set_ui_file_fputs (ui_file, tee_file_fputs);
  set_ui_file_isatty (ui_file, tee_file_isatty);
  return ui_file;
}

static void
tee_file_delete (struct ui_file *file)
{
  struct tee_file *tee = ui_file_data (file);

  if (tee->magic != &tee_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("tee_file_delete: bad magic number"));
  if (tee->close_one)
    ui_file_delete (tee->one);
  if (tee->close_two)
    ui_file_delete (tee->two);

  xfree (tee);
}

static void
tee_file_flush (struct ui_file *file)
{
  struct tee_file *tee = ui_file_data (file);

  if (tee->magic != &tee_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("tee_file_flush: bad magic number"));
  tee->one->to_flush (tee->one);
  tee->two->to_flush (tee->two);
}

static void
tee_file_write (struct ui_file *file, const char *buf, long length_buf)
{
  struct tee_file *tee = ui_file_data (file);

  if (tee->magic != &tee_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("tee_file_write: bad magic number"));
  ui_file_write (tee->one, buf, length_buf);
  ui_file_write (tee->two, buf, length_buf);
}

static void
tee_file_fputs (const char *linebuffer, struct ui_file *file)
{
  struct tee_file *tee = ui_file_data (file);

  if (tee->magic != &tee_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("tee_file_fputs: bad magic number"));
  tee->one->to_fputs (linebuffer, tee->one);
  tee->two->to_fputs (linebuffer, tee->two);
}

static int
tee_file_isatty (struct ui_file *file)
{
  struct tee_file *tee = ui_file_data (file);

  if (tee->magic != &tee_file_magic)
    internal_error (__FILE__, __LINE__,
		    _("tee_file_isatty: bad magic number"));

  return ui_file_isatty (tee->one);
}
