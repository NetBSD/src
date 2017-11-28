/* Exception (throw catch) mechanism, for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "ui-out.h"

/* If E is an exception, print it's error message on the specified
   stream.  For _fprintf, prefix the message with PREFIX...  */
extern void exception_print (struct ui_file *file, struct gdb_exception e);
extern void exception_fprintf (struct ui_file *file, struct gdb_exception e,
			       const char *prefix,
			       ...) ATTRIBUTE_PRINTF (3, 4);

/* Call FUNC(UIOUT, FUNC_ARGS) but wrapped within an exception
   handler.  If an exception (enum return_reason) is thrown using
   throw_exception() than all cleanups installed since
   catch_exceptions() was entered are invoked, the (-ve) exception
   value is then returned by catch_exceptions.  If FUNC() returns
   normally (with a positive or zero return value) then that value is
   returned by catch_exceptions().  It is an internal_error() for
   FUNC() to return a negative value.

   For the period of the FUNC() call: UIOUT is installed as the output
   builder; ERRSTRING is installed as the error/quit message; and a
   new cleanup_chain is established.  The old values are restored
   before catch_exceptions() returns.

   The variant catch_exceptions_with_msg() is the same as
   catch_exceptions() but adds the ability to return an allocated
   copy of the gdb error message.  This is used when a silent error is 
   issued and the caller wants to manually issue the error message.

   MASK specifies what to catch; it is normally set to
   RETURN_MASK_ALL, if for no other reason than that the code which
   calls catch_errors might not be set up to deal with a quit which
   isn't caught.  But if the code can deal with it, it generally
   should be RETURN_MASK_ERROR, unless for some reason it is more
   useful to abort only the portion of the operation inside the
   catch_errors.  Note that quit should return to the command line
   fairly quickly, even if some further processing is being done.

   FIXME; cagney/2001-08-13: The need to override the global UIOUT
   builder variable should just go away.

   This function supersedes catch_errors().

   This function uses SETJMP() and LONGJUMP().  */

struct ui_out;
typedef int (catch_exceptions_ftype) (struct ui_out *ui_out, void *args);
extern int catch_exceptions (struct ui_out *uiout,
			     catch_exceptions_ftype *func, void *func_args,
			     return_mask mask);
typedef void (catch_exception_ftype) (struct ui_out *ui_out, void *args);
extern int catch_exceptions_with_msg (struct ui_out *uiout,
			     	      catch_exceptions_ftype *func, 
			     	      void *func_args,
			     	      char **gdberrmsg,
				      return_mask mask);

/* If CATCH_ERRORS_FTYPE throws an error, catch_errors() returns zero
   otherwize the result from CATCH_ERRORS_FTYPE is returned.  It is
   probably useful for CATCH_ERRORS_FTYPE to always return a non-zero
   value.  It's unfortunate that, catch_errors() does not return an
   indication of the exact exception that it caught - quit_flag might
   help.

   This function is superseded by catch_exceptions().  */

typedef int (catch_errors_ftype) (void *);
extern int catch_errors (catch_errors_ftype *, void *, char *, return_mask);

#endif
