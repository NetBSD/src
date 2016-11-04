/* Exception (throw catch) mechanism, for GDB, the GNU debugger.

   Copyright (C) 1986-2015 Free Software Foundation, Inc.

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

#include "gdb_setjmp.h"

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

  /* DW_OP_GNU_entry_value resolving failed.  */
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

/* Functions to drive the exceptions state machine.  Though declared
   here by necessity, these functions should be considered internal to
   the exceptions subsystem and not used other than via the TRY/CATCH
   macros defined below.  */

#ifndef __cplusplus
extern SIGJMP_BUF *exceptions_state_mc_init (void);
extern int exceptions_state_mc_action_iter (void);
extern int exceptions_state_mc_action_iter_1 (void);
extern int exceptions_state_mc_catch (struct gdb_exception *, int);
#else
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

  */

#ifndef __cplusplus

#define TRY \
     { \
       SIGJMP_BUF *buf = \
	 exceptions_state_mc_init (); \
       SIGSETJMP (*buf); \
     } \
     while (exceptions_state_mc_action_iter ()) \
       while (exceptions_state_mc_action_iter_1 ())

#define CATCH(EXCEPTION, MASK)				\
  {							\
    struct gdb_exception EXCEPTION;				\
    if (exceptions_state_mc_catch (&(EXCEPTION), MASK))

#define END_CATCH				\
  }

#else

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

#endif

/* *INDENT-ON* */

/* Hook to allow client-specific actions to be performed prior to
   throwing an exception.  This function must be provided by the
   client, and will be called before any cleanups are run.  */

extern void prepare_to_throw_exception (void);

/* Throw an exception (as described by "struct gdb_exception").  Will
   execute a LONG JUMP to the inner most containing exception handler
   established using catch_exceptions() (or similar).

   Code normally throws an exception using error() et.al.  For various
   reaons, GDB also contains code that throws an exception directly.
   For instance, the remote*.c targets contain CNTRL-C signal handlers
   that propogate the QUIT event up the exception chain.  ``This could
   be a good thing or a dangerous thing.'' -- the Existential
   Wombat.  */

extern void throw_exception (struct gdb_exception exception)
     ATTRIBUTE_NORETURN;
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
