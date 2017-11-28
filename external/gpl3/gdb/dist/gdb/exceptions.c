/* Exception (throw catch) mechanism, for GDB, the GNU debugger.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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
#include "exceptions.h"
#include "breakpoint.h"
#include "target.h"
#include "inferior.h"
#include "annotate.h"
#include "ui-out.h"
#include "serial.h"
#include "gdbthread.h"
#include "top.h"

static void
print_flush (void)
{
  struct ui *ui = current_ui;
  struct serial *gdb_stdout_serial;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);

  if (deprecated_error_begin_hook)
    deprecated_error_begin_hook ();

  if (target_supports_terminal_ours ())
    {
      make_cleanup_restore_target_terminal ();
      target_terminal_ours_for_output ();
    }

  /* We want all output to appear now, before we print the error.  We
     have 3 levels of buffering we have to flush (it's possible that
     some of these should be changed to flush the lower-level ones
     too):  */

  /* 1.  The _filtered buffer.  */
  if (filtered_printing_initialized ())
    wrap_here ("");

  /* 2.  The stdio buffer.  */
  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  /* 3.  The system-level buffer.  */
  gdb_stdout_serial = serial_fdopen (fileno (ui->outstream));
  if (gdb_stdout_serial)
    {
      serial_drain_output (gdb_stdout_serial);
      serial_un_fdopen (gdb_stdout_serial);
    }

  annotate_error_begin ();

  do_cleanups (old_chain);
}

static void
print_exception (struct ui_file *file, struct gdb_exception e)
{
  /* KLUGE: cagney/2005-01-13: Write the string out one line at a time
     as that way the MI's behavior is preserved.  */
  const char *start;
  const char *end;

  for (start = e.message; start != NULL; start = end)
    {
      end = strchr (start, '\n');
      if (end == NULL)
	fputs_filtered (start, file);
      else
	{
	  end++;
	  ui_file_write (file, start, end - start);
	}
    }					    
  fprintf_filtered (file, "\n");

  /* Now append the annotation.  */
  switch (e.reason)
    {
    case RETURN_QUIT:
      annotate_quit ();
      break;
    case RETURN_ERROR:
      /* Assume that these are all errors.  */
      annotate_error ();
      break;
    default:
      internal_error (__FILE__, __LINE__, _("Bad switch."));
    }
}

void
exception_print (struct ui_file *file, struct gdb_exception e)
{
  if (e.reason < 0 && e.message != NULL)
    {
      print_flush ();
      print_exception (file, e);
    }
}

void
exception_fprintf (struct ui_file *file, struct gdb_exception e,
		   const char *prefix, ...)
{
  if (e.reason < 0 && e.message != NULL)
    {
      va_list args;

      print_flush ();

      /* Print the prefix.  */
      va_start (args, prefix);
      vfprintf_filtered (file, prefix, args);
      va_end (args);

      print_exception (file, e);
    }
}

/* Call FUNC(UIOUT, FUNC_ARGS) but wrapped within an exception
   handler.  If an exception (enum return_reason) is thrown using
   throw_exception() than all cleanups installed since
   catch_exceptions() was entered are invoked, the (-ve) exception
   value is then returned by catch_exceptions.  If FUNC() returns
   normally (with a positive or zero return value) then that value is
   returned by catch_exceptions().  It is an internal_error() for
   FUNC() to return a negative value.

   See exceptions.h for further usage details.  */

/* MAYBE: cagney/1999-11-05: catch_errors() in conjunction with
   error() et al. could maintain a set of flags that indicate the
   current state of each of the longjmp buffers.  This would give the
   longjmp code the chance to detect a longjmp botch (before it gets
   to longjmperror()).  Prior to 1999-11-05 this wasn't possible as
   code also randomly used a SET_TOP_LEVEL macro that directly
   initialized the longjmp buffers.  */

int
catch_exceptions (struct ui_out *uiout,
		  catch_exceptions_ftype *func,
		  void *func_args,
		  return_mask mask)
{
  return catch_exceptions_with_msg (uiout, func, func_args, NULL, mask);
}

int
catch_exceptions_with_msg (struct ui_out *func_uiout,
		  	   catch_exceptions_ftype *func,
		  	   void *func_args,
			   char **gdberrmsg,
		  	   return_mask mask)
{
  struct gdb_exception exception = exception_none;
  volatile int val = 0;
  struct ui_out *saved_uiout;

  /* Save and override the global ``struct ui_out'' builder.  */
  saved_uiout = current_uiout;
  current_uiout = func_uiout;

  TRY
    {
      val = (*func) (current_uiout, func_args);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception = ex;
    }
  END_CATCH

  /* Restore the global builder.  */
  current_uiout = saved_uiout;

  if (exception.reason < 0 && (mask & RETURN_MASK (exception.reason)) == 0)
    {
      /* The caller didn't request that the event be caught.
	 Rethrow.  */
      throw_exception (exception);
    }

  exception_print (gdb_stderr, exception);
  gdb_assert (val >= 0);
  gdb_assert (exception.reason <= 0);
  if (exception.reason < 0)
    {
      /* If caller wants a copy of the low-level error message, make
	 one.  This is used in the case of a silent error whereby the
	 caller may optionally want to issue the message.  */
      if (gdberrmsg != NULL)
	{
	  if (exception.message != NULL)
	    *gdberrmsg = xstrdup (exception.message);
	  else
	    *gdberrmsg = NULL;
	}
      return exception.reason;
    }
  return val;
}

/* This function is superseded by catch_exceptions().  */

int
catch_errors (catch_errors_ftype *func, void *func_args,
	      const char *errstring, return_mask mask)
{
  struct gdb_exception exception = exception_none;
  volatile int val = 0;
  struct ui_out *saved_uiout;

  /* Save the global ``struct ui_out'' builder.  */
  saved_uiout = current_uiout;

  TRY
    {
      val = func (func_args);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception = ex;
    }
  END_CATCH

  /* Restore the global builder.  */
  current_uiout = saved_uiout;

  if (exception.reason < 0 && (mask & RETURN_MASK (exception.reason)) == 0)
    {
      /* The caller didn't request that the event be caught.
	 Rethrow.  */
      throw_exception (exception);
    }

  exception_fprintf (gdb_stderr, exception, "%s", errstring);
  if (exception.reason != 0)
    return 0;
  return val;
}

/* See exceptions.h.  */

int
exception_print_same (struct gdb_exception e1, struct gdb_exception e2)
{
  const char *msg1 = e1.message;
  const char *msg2 = e2.message;

  if (msg1 == NULL)
    msg1 = "";
  if (msg2 == NULL)
    msg2 = "";

  return (e1.reason == e2.reason
	  && e1.error == e2.error
	  && strcmp (msg1, msg2) == 0);
}
