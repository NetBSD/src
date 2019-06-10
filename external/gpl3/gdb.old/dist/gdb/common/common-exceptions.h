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

#ifndef COMMON_EXCEPTIONS_H
#define COMMON_EXCEPTIONS_H

#include <setjmp.h>
#include <new>

/* Reasons for calling throw_exceptions().  NOTE: all reason values
   must be less than zero.  enum value 0 is reserved for internal use
   as the return value from an initial setjmp().  The function
   catch_exceptions() reserves values >= 0 as legal results from its
   wrapped function.  */

enum return_reason
  {
    /* User interrupt.  */
    RETURN_QUIT = -2,
    /* Any other error.  */
    RETURN_ERROR
  };

#define RETURN_MASK(reason)	(1 << (int)(-reason))

typedef enum
{
  RETURN_MASK_QUIT = RETURN_MASK (RETURN_QUIT),
  RETURN_MASK_ERROR = RETURN_MASK (RETURN_ERROR),
  RETURN_MASK_ALL = (RETURN_MASK_QUIT | RETURN_MASK_ERROR)
} return_mask;

/* Describe all exceptions.  */

enum errors {
  GDB_NO_ERROR,

  /* Any generic error, the corresponding text is in
     exception.message.  */
  GENERIC_ERROR,

  /* Something requested was not found.  */
  NOT_FOUND_ERROR,

  /* Thread library lacks support necessary for finding thread local
     storage.  */
  TLS_NO_LIBRARY_SUPPORT_ERROR,

  /* Load module not found while attempting to find thread local storage.  */
  TLS_LOAD_MODULE_NOT_FOUND_ERROR,

  /* Thread local storage has not been allocated yet.  */
  TLS_NOT_ALLOCATED_YET_ERROR,

  /* Something else went wrong while attempting to find thread local
     storage.  The ``struct gdb_exception'' message field provides
     more detail.  */
  TLS_GENERIC_ERROR,

  /* Problem parsing an XML document.  */
  XML_PARSE_ERROR,

  /* Error accessing memory.  */
  MEMORY_ERROR,

  /* Value not available.  E.g., a register was not collected in a
     traceframe.  */
  NOT_AVAILABLE_ERROR,

  /* Value was optimized out.  Note: if the value was a register, this
     means the register was not saved in the frame.  */
  OPTIMIZED_OUT_ERROR,

  /* DW_OP_entry_value resolving failed.  */
  NO_ENTRY_VALUE_ERROR,

  /* Target throwing an error has been closed.  Current command should be
     aborted as the inferior state is no longer valid.  */
  TARGET_CLOSE_ERROR,

  /* An undefined command was executed.  */
  UNDEFINED_COMMAND_ERROR,

  /* Requested feature, method, mechanism, etc. is not supported.  */
  NOT_SUPPORTED_ERROR,

  /* The number of candidates generated during line completion has
     reached the user's specified limit.  This isn't an error, this exception
     is used to halt searching for more completions, but for consistency
     "_ERROR" is appended to the name.  */
  MAX_COMPLETIONS_REACHED_ERROR,

  /* Add more errors here.  */
  NR_ERRORS
};

struct gdb_exception
{
  enum return_reason reason;
  enum errors error;
  const char *message;
};

/* The different exception mechanisms that TRY/CATCH can map to.  */

/* Make GDB exceptions use setjmp/longjmp behind the scenes.  */
#define GDB_XCPT_SJMP 1

/* Make GDB exceptions use try/catch behind the scenes.  */
#define GDB_XCPT_TRY 2

/* Specify this mode to build with TRY/CATCH mapped directly to raw
   try/catch.  GDB won't work correctly, but building that way catches
   code tryin to break/continue out of the try block, along with
   spurious code between the TRY and the CATCH block.  */
#define GDB_XCPT_RAW_TRY 3

#define GDB_XCPT GDB_XCPT_TRY

/* Functions to drive the sjlj-based exceptions state machine.  Though
   declared here by necessity, these functions should be considered
   internal to the exceptions subsystem and not used other than via
   the TRY/CATCH (or TRY_SJLJ/CATCH_SJLJ) macros defined below.  */

extern jmp_buf *exceptions_state_mc_init (void);
extern int exceptions_state_mc_action_iter (void);
extern int exceptions_state_mc_action_iter_1 (void);
extern int exceptions_state_mc_catch (struct gdb_exception *, int);

/* Same, but for the C++ try/catch-based TRY/CATCH mechanism.  */

#if GDB_XCPT != GDB_XCPT_SJMP
extern void *exception_try_scope_entry (void);
extern void exception_try_scope_exit (void *saved_state);
extern void exception_rethrow (void);
#endif

/* Macro to wrap up standard try/catch behavior.

   The double loop lets us correctly handle code "break"ing out of the
   try catch block.  (It works as the "break" only exits the inner
   "while" loop, the outer for loop detects this handling it
   correctly.)  Of course "return" and "goto" are not so lucky.

   For instance:

   *INDENT-OFF*

   TRY
     {
     }
   CATCH (e, RETURN_MASK_ERROR)
     {
       switch (e.reason)
         {
           case RETURN_ERROR: ...
         }
     }
   END_CATCH

  Note that the SJLJ version of the macros are actually named
  TRY_SJLJ/CATCH_SJLJ in order to make it possible to call them even
  when TRY/CATCH are mapped to C++ try/catch.  The SJLJ variants are
  needed in some cases where gdb exceptions need to cross third-party
  library code compiled without exceptions support (e.g.,
  readline).  */

#define TRY_SJLJ \
     { \
       jmp_buf *buf = \
	 exceptions_state_mc_init (); \
       setjmp (*buf); \
     } \
     while (exceptions_state_mc_action_iter ()) \
       while (exceptions_state_mc_action_iter_1 ())

#define CATCH_SJLJ(EXCEPTION, MASK)				\
  {							\
    struct gdb_exception EXCEPTION;				\
    if (exceptions_state_mc_catch (&(EXCEPTION), MASK))

#define END_CATCH_SJLJ				\
  }

#if GDB_XCPT == GDB_XCPT_SJMP

/* If using SJLJ-based exceptions for all exceptions, then provide
   standard aliases.  */

#define TRY TRY_SJLJ
#define CATCH CATCH_SJLJ
#define END_CATCH END_CATCH_SJLJ

#endif /* GDB_XCPT_SJMP */

#if GDB_XCPT == GDB_XCPT_TRY || GDB_XCPT == GDB_XCPT_RAW_TRY

/* Prevent error/quit during TRY from calling cleanups established
   prior to here.  This pops out the scope in either case of normal
   exit or exception exit.  */
struct exception_try_scope
{
  exception_try_scope ()
  {
    saved_state = exception_try_scope_entry ();
  }
  ~exception_try_scope ()
  {
    exception_try_scope_exit (saved_state);
  }

  void *saved_state;
};

#if GDB_XCPT == GDB_XCPT_TRY

/* We still need to wrap TRY/CATCH in C++ so that cleanups and C++
   exceptions can coexist.  The TRY blocked is wrapped in a
   do/while(0) so that break/continue within the block works the same
   as in C.  */
#define TRY								\
  try									\
    {									\
      exception_try_scope exception_try_scope_instance;			\
      do								\
	{

#define CATCH(EXCEPTION, MASK)						\
	} while (0);							\
    }								        \
  catch (struct gdb_exception ## _ ## MASK &EXCEPTION)

#define END_CATCH				\
  catch (...)					\
  {						\
    exception_rethrow ();			\
  }

#else

#define TRY try
#define CATCH(EXCEPTION, MASK) \
  catch (struct gdb_exception ## _ ## MASK &EXCEPTION)
#define END_CATCH

#endif

/* The exception types client code may catch.  They're just shims
   around gdb_exception that add nothing but type info.  Which is used
   is selected depending on the MASK argument passed to CATCH.  */

struct gdb_exception_RETURN_MASK_ALL : public gdb_exception
{
};

struct gdb_exception_RETURN_MASK_ERROR : public gdb_exception_RETURN_MASK_ALL
{
};

struct gdb_exception_RETURN_MASK_QUIT : public gdb_exception_RETURN_MASK_ALL
{
};

#endif /* GDB_XCPT_TRY || GDB_XCPT_RAW_TRY */

/* An exception type that inherits from both std::bad_alloc and a gdb
   exception.  This is necessary because operator new can only throw
   std::bad_alloc, and OTOH, we want exceptions thrown due to memory
   allocation error to be caught by all the CATCH/RETURN_MASK_ALL
   spread around the codebase.  */

struct gdb_quit_bad_alloc
  : public gdb_exception_RETURN_MASK_QUIT,
    public std::bad_alloc
{
  explicit gdb_quit_bad_alloc (gdb_exception ex)
    : std::bad_alloc ()
  {
    gdb_exception *self = this;

    *self = ex;
  }
};

/* *INDENT-ON* */

/* Throw an exception (as described by "struct gdb_exception"),
   landing in the inner most containing exception handler established
   using TRY/CATCH.  */
extern void throw_exception (struct gdb_exception exception)
     ATTRIBUTE_NORETURN;

/* Throw an exception by executing a LONG JUMP to the inner most
   containing exception handler established using TRY_SJLJ.  Necessary
   in some cases where we need to throw GDB exceptions across
   third-party library code (e.g., readline).  */
extern void throw_exception_sjlj (struct gdb_exception exception)
     ATTRIBUTE_NORETURN;

/* Convenience wrappers around throw_exception that throw GDB
   errors.  */
extern void throw_verror (enum errors, const char *fmt, va_list ap)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (2, 0);
extern void throw_vquit (const char *fmt, va_list ap)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 0);
extern void throw_error (enum errors error, const char *fmt, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (2, 3);
extern void throw_quit (const char *fmt, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);

/* A pre-defined non-exception.  */
extern const struct gdb_exception exception_none;

#endif /* COMMON_EXCEPTIONS_H */
