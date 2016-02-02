/* *INDENT-OFF* */ /* ATTRIBUTE_PRINTF confuses indent, avoid running it
		      for now.  */
/* Basic, host-specific, and target-specific definitions for GDB.
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

#ifndef DEFS_H
#define DEFS_H

#ifdef GDBSERVER
#  error gdbserver should not include gdb/defs.h
#endif

#include "common-defs.h"

#include <sys/types.h>
#include <limits.h>
#include <stdint.h>

/* The libdecnumber library, on which GDB depends, includes a header file
   called gstdint.h instead of relying directly on stdint.h.  GDB, on the
   other hand, includes stdint.h directly, relying on the fact that gnulib
   generates a copy if the system doesn't provide one or if it is missing
   some features.  Unfortunately, gstdint.h and stdint.h cannot be included
   at the same time, which may happen when we include a file from
   libdecnumber.

   The following macro definition effectively prevents the inclusion of
   gstdint.h, as all the definitions it provides are guarded against
   the GCC_GENERATED_STDINT_H macro.  We already have gnulib/stdint.h
   included, so it's ok to blank out gstdint.h.  */
#define GCC_GENERATED_STDINT_H 1

#include <unistd.h>

#include <fcntl.h>

#include "gdb_wchar.h"

#include "ui-file.h"

#include "host-defs.h"

/* Scope types enumerator.  List the types of scopes the compiler will
   accept.  */

enum compile_i_scope_types
  {
    COMPILE_I_INVALID_SCOPE,

    /* A simple scope.  Wrap an expression into a simple scope that
       takes no arguments, returns no value, and uses the generic
       function name "_gdb_expr". */

    COMPILE_I_SIMPLE_SCOPE,

    /* Do not wrap the expression,
       it has to provide function "_gdb_expr" on its own.  */
    COMPILE_I_RAW_SCOPE,
  };

/* Just in case they're not defined in stdio.h.  */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

/* The O_BINARY flag is defined in fcntl.h on some non-Posix platforms.
   It is used as an access modifier in calls to open(), where it acts
   similarly to the "b" character in fopen()'s MODE argument.  On Posix
   platforms it should be a no-op, so it is defined as 0 here.  This 
   ensures that the symbol may be used freely elsewhere in gdb.  */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include "hashtab.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* * Enable xdb commands if set.  */
extern int xdb_commands;

/* * Enable dbx commands if set.  */
extern int dbx_commands;

/* * System root path, used to find libraries etc.  */
extern char *gdb_sysroot;

/* * GDB datadir, used to store data files.  */
extern char *gdb_datadir;

/* * If non-NULL, the possibly relocated path to python's "lib" directory
   specified with --with-python.  */
extern char *python_libdir;

/* * Search path for separate debug files.  */
extern char *debug_file_directory;

/* GDB has two methods for handling SIGINT.  When immediate_quit is
   nonzero, a SIGINT results in an immediate longjmp out of the signal
   handler.  Otherwise, SIGINT simply sets a flag; code that might
   take a long time, and which ought to be interruptible, checks this
   flag using the QUIT macro.

   These functions use the extension_language_ops API to allow extension
   language(s) and GDB SIGINT handling to coexist seamlessly.  */

/* * Clear the quit flag.  */
extern void clear_quit_flag (void);
/* * Evaluate to non-zero if the quit flag is set, zero otherwise.  This
   will clear the quit flag as a side effect.  */
extern int check_quit_flag (void);
/* * Set the quit flag.  */
extern void set_quit_flag (void);

/* Flag that function quit should call quit_force.  */
extern volatile int sync_quit_force_run;

extern int immediate_quit;

extern void quit (void);

/* FIXME: cagney/2000-03-13: It has been suggested that the peformance
   benefits of having a ``QUIT'' macro rather than a function are
   marginal.  If the overhead of a QUIT function call is proving
   significant then its calling frequency should probably be reduced
   [kingdon].  A profile analyzing the current situtation is
   needed.  */

#define QUIT { \
  if (check_quit_flag () || sync_quit_force_run) quit (); \
  if (deprecated_interactive_hook) deprecated_interactive_hook (); \
}

/* * Languages represented in the symbol table and elsewhere.
   This should probably be in language.h, but since enum's can't
   be forward declared to satisfy opaque references before their
   actual definition, needs to be here.  */

enum language
  {
    language_unknown,		/* Language not known */
    language_auto,		/* Placeholder for automatic setting */
    language_c,			/* C */
    language_cplus,		/* C++ */
    language_d,			/* D */
    language_go,		/* Go */
    language_objc,		/* Objective-C */
    language_java,		/* Java */
    language_fortran,		/* Fortran */
    language_m2,		/* Modula-2 */
    language_asm,		/* Assembly language */
    language_pascal,		/* Pascal */
    language_ada,		/* Ada */
    language_opencl,		/* OpenCL */
    language_minimal,		/* All other languages, minimal support only */
    nr_languages
  };

enum precision_type
  {
    single_precision,
    double_precision,
    unspecified_precision
  };

/* * A generic, not quite boolean, enumeration.  This is used for
   set/show commands in which the options are on/off/automatic.  */
enum auto_boolean
{
  AUTO_BOOLEAN_TRUE,
  AUTO_BOOLEAN_FALSE,
  AUTO_BOOLEAN_AUTO
};

/* * Potential ways that a function can return a value of a given
   type.  */

enum return_value_convention
{
  /* * Where the return value has been squeezed into one or more
     registers.  */
  RETURN_VALUE_REGISTER_CONVENTION,
  /* * Commonly known as the "struct return convention".  The caller
     passes an additional hidden first parameter to the caller.  That
     parameter contains the address at which the value being returned
     should be stored.  While typically, and historically, used for
     large structs, this is convention is applied to values of many
     different types.  */
  RETURN_VALUE_STRUCT_CONVENTION,
  /* * Like the "struct return convention" above, but where the ABI
     guarantees that the called function stores the address at which
     the value being returned is stored in a well-defined location,
     such as a register or memory slot in the stack frame.  Don't use
     this if the ABI doesn't explicitly guarantees this.  */
  RETURN_VALUE_ABI_RETURNS_ADDRESS,
  /* * Like the "struct return convention" above, but where the ABI
     guarantees that the address at which the value being returned is
     stored will be available in a well-defined location, such as a
     register or memory slot in the stack frame.  Don't use this if
     the ABI doesn't explicitly guarantees this.  */
  RETURN_VALUE_ABI_PRESERVES_ADDRESS,
};

/* Needed for various prototypes */

struct symtab;
struct breakpoint;
struct frame_info;
struct gdbarch;
struct value;

/* From main.c.  */

/* This really belong in utils.c (path-utils.c?), but it references some
   globals that are currently only available to main.c.  */
extern char *relocate_gdb_directory (const char *initial, int flag);


/* Annotation stuff.  */

extern int annotation_level;	/* in stack.c */


/* From regex.c or libc.  BSD 4.4 declares this with the argument type as
   "const char *" in unistd.h, so we can't declare the argument
   as "char *".  */

extern char *re_comp (const char *);

/* From symfile.c */

extern void symbol_file_command (char *, int);

/* * Remote targets may wish to use this as their load function.  */
extern void generic_load (const char *name, int from_tty);

/* * Report on STREAM the performance of memory transfer operation,
   such as 'load'.
   @param DATA_COUNT is the number of bytes transferred.
   @param WRITE_COUNT is the number of separate write operations, or 0,
   if that information is not available.
   @param START_TIME is the time at which an operation was started.
   @param END_TIME is the time at which an operation ended.  */
struct timeval;
extern void print_transfer_performance (struct ui_file *stream,
					unsigned long data_count,
					unsigned long write_count,
					const struct timeval *start_time,
					const struct timeval *end_time);

/* From top.c */

typedef void initialize_file_ftype (void);

extern char *gdb_readline (const char *);

extern char *gdb_readline_wrapper (const char *);

extern char *command_line_input (const char *, int, char *);

extern void print_prompt (void);

extern int input_from_terminal_p (void);

extern int info_verbose;

/* From printcmd.c */

extern void set_next_address (struct gdbarch *, CORE_ADDR);

extern int print_address_symbolic (struct gdbarch *, CORE_ADDR,
				   struct ui_file *, int, char *);

extern int build_address_symbolic (struct gdbarch *,
				   CORE_ADDR addr,
				   int do_demangle, 
				   char **name, 
				   int *offset, 
				   char **filename, 
				   int *line, 	
				   int *unmapped);

extern void print_address (struct gdbarch *, CORE_ADDR, struct ui_file *);
extern const char *pc_prefix (CORE_ADDR);

/* From source.c */

/* See openp function definition for their description.  */
#define OPF_TRY_CWD_FIRST     0x01
#define OPF_SEARCH_IN_PATH    0x02
#define OPF_RETURN_REALPATH   0x04

extern int openp (const char *, int, const char *, int, char **);

extern int source_full_path_of (const char *, char **);

extern void mod_path (char *, char **);

extern void add_path (char *, char **, int);

extern void directory_switch (char *, int);

extern char *source_path;

extern void init_source_path (void);

/* From exec.c */

/* * Process memory area starting at ADDR with length SIZE.  Area is
   readable iff READ is non-zero, writable if WRITE is non-zero,
   executable if EXEC is non-zero.  Area is possibly changed against
   its original file based copy if MODIFIED is non-zero.  DATA is
   passed without changes from a caller.  */

typedef int (*find_memory_region_ftype) (CORE_ADDR addr, unsigned long size,
					 int read, int write, int exec,
					 int modified, void *data);

/* * Possible lvalue types.  Like enum language, this should be in
   value.h, but needs to be here for the same reason.  */

enum lval_type
  {
    /* * Not an lval.  */
    not_lval,
    /* * In memory.  */
    lval_memory,
    /* * In a register.  Registers are relative to a frame.  */
    lval_register,
    /* * In a gdb internal variable.  */
    lval_internalvar,
    /* * Value encapsulates a callable defined in an extension language.  */
    lval_xcallable,
    /* * Part of a gdb internal variable (structure field).  */
    lval_internalvar_component,
    /* * Value's bits are fetched and stored using functions provided
       by its creator.  */
    lval_computed
  };

/* * Control types for commands.  */

enum misc_command_type
  {
    ok_command,
    end_command,
    else_command,
    nop_command
  };

enum command_control_type
  {
    simple_control,
    break_control,
    continue_control,
    while_control,
    if_control,
    commands_control,
    python_control,
    compile_control,
    guile_control,
    while_stepping_control,
    invalid_control
  };

/* * Structure for saved commands lines (for breakpoints, defined
   commands, etc).  */

struct command_line
  {
    struct command_line *next;
    char *line;
    enum command_control_type control_type;
    union
      {
	struct
	  {
	    enum compile_i_scope_types scope;
	  }
	compile;
      }
    control_u;
    /* * The number of elements in body_list.  */
    int body_count;
    /* * For composite commands, the nested lists of commands.  For
       example, for "if" command this will contain the then branch and
       the else branch, if that is available.  */
    struct command_line **body_list;
  };

extern struct command_line *read_command_lines (char *, int, int,
						void (*)(char *, void *),
						void *);
extern struct command_line *read_command_lines_1 (char * (*) (void), int,
						  void (*)(char *, void *),
						  void *);

extern void free_command_lines (struct command_line **);

/* * Parameters of the "info proc" command.  */

enum info_proc_what
  {
    /* * Display the default cmdline, cwd and exe outputs.  */
    IP_MINIMAL,

    /* * Display `info proc mappings'.  */
    IP_MAPPINGS,

    /* * Display `info proc status'.  */
    IP_STATUS,

    /* * Display `info proc stat'.  */
    IP_STAT,

    /* * Display `info proc cmdline'.  */
    IP_CMDLINE,

    /* * Display `info proc exe'.  */
    IP_EXE,

    /* * Display `info proc cwd'.  */
    IP_CWD,

    /* * Display all of the above.  */
    IP_ALL
  };

/* * String containing the current directory (what getwd would return).  */

extern char *current_directory;

/* * Default radixes for input and output.  Only some values supported.  */
extern unsigned input_radix;
extern unsigned output_radix;

/* * Possibilities for prettyformat parameters to routines which print
   things.  Like enum language, this should be in value.h, but needs
   to be here for the same reason.  FIXME:  If we can eliminate this
   as an arg to LA_VAL_PRINT, then we can probably move it back to
   value.h.  */

enum val_prettyformat
  {
    Val_no_prettyformat = 0,
    Val_prettyformat,
    /* * Use the default setting which the user has specified.  */
    Val_prettyformat_default
  };

/* * Optional native machine support.  Non-native (and possibly pure
   multi-arch) targets do not need a "nm.h" file.  This will be a
   symlink to one of the nm-*.h files, built by the `configure'
   script.  */

#ifdef GDB_NM_FILE
#include "nm.h"
#endif

/* Assume that fopen accepts the letter "b" in the mode string.
   It is demanded by ISO C9X, and should be supported on all
   platforms that claim to have a standard-conforming C library.  On
   true POSIX systems it will be ignored and have no effect.  There
   may still be systems without a standard-conforming C library where
   an ISO C9X compiler (GCC) is available.  Known examples are SunOS
   4.x and 4.3BSD.  This assumption means these systems are no longer
   supported.  */
#ifndef FOPEN_RB
# include "fopen-bin.h"
#endif

/* Defaults for system-wide constants (if not defined by xm.h, we fake it).
   FIXME: Assumes 2's complement arithmetic.  */

#if !defined (UINT_MAX)
#define	UINT_MAX ((unsigned int)(~0))	    /* 0xFFFFFFFF for 32-bits */
#endif

#if !defined (INT_MAX)
#define	INT_MAX ((int)(UINT_MAX >> 1))	    /* 0x7FFFFFFF for 32-bits */
#endif

#if !defined (INT_MIN)
#define INT_MIN ((int)((int) ~0 ^ INT_MAX)) /* 0x80000000 for 32-bits */
#endif

#if !defined (ULONG_MAX)
#define	ULONG_MAX ((unsigned long)(~0L))    /* 0xFFFFFFFF for 32-bits */
#endif

#if !defined (LONG_MAX)
#define	LONG_MAX ((long)(ULONG_MAX >> 1))   /* 0x7FFFFFFF for 32-bits */
#endif

#if !defined (ULONGEST_MAX)
#define	ULONGEST_MAX (~(ULONGEST)0)        /* 0xFFFFFFFFFFFFFFFF for 64-bits */
#endif

#if !defined (LONGEST_MAX)                 /* 0x7FFFFFFFFFFFFFFF for 64-bits */
#define	LONGEST_MAX ((LONGEST)(ULONGEST_MAX >> 1))
#endif

/* * Convert a LONGEST to an int.  This is used in contexts (e.g. number of
   arguments to a function, number in a value history, register number, etc.)
   where the value must not be larger than can fit in an int.  */

extern int longest_to_int (LONGEST);

/* * List of known OS ABIs.  If you change this, make sure to update the
   table in osabi.c.  */
enum gdb_osabi
{
  GDB_OSABI_UNINITIALIZED = -1, /* For struct gdbarch_info.  */

  GDB_OSABI_UNKNOWN = 0,	/* keep this zero */

  GDB_OSABI_SVR4,
  GDB_OSABI_HURD,
  GDB_OSABI_SOLARIS,
  GDB_OSABI_LINUX,
  GDB_OSABI_FREEBSD_AOUT,
  GDB_OSABI_FREEBSD_ELF,
  GDB_OSABI_NETBSD_AOUT,
  GDB_OSABI_NETBSD_ELF,
  GDB_OSABI_OPENBSD_ELF,
  GDB_OSABI_WINCE,
  GDB_OSABI_GO32,
  GDB_OSABI_IRIX,
  GDB_OSABI_HPUX_ELF,
  GDB_OSABI_HPUX_SOM,
  GDB_OSABI_QNXNTO,
  GDB_OSABI_CYGWIN,
  GDB_OSABI_AIX,
  GDB_OSABI_DICOS,
  GDB_OSABI_DARWIN,
  GDB_OSABI_SYMBIAN,
  GDB_OSABI_OPENVMS,
  GDB_OSABI_LYNXOS178,
  GDB_OSABI_NEWLIB,
  GDB_OSABI_SDE,

  GDB_OSABI_INVALID		/* keep this last */
};

/* Global functions from other, non-gdb GNU thingies.
   Libiberty thingies are no longer declared here.  We include libiberty.h
   above, instead.  */

/* From other system libraries */

#ifndef atof
extern double atof (const char *);	/* X3.159-1989  4.10.1.1 */
#endif

/* Dynamic target-system-dependent parameters for GDB.  */
#include "gdbarch.h"

/* * Maximum size of a register.  Something small, but large enough for
   all known ISAs.  If it turns out to be too small, make it bigger.  */

enum { MAX_REGISTER_SIZE = 64 };

/* Static target-system-dependent parameters for GDB.  */

/* * Number of bits in a char or unsigned char for the target machine.
   Just like CHAR_BIT in <limits.h> but describes the target machine.  */
#if !defined (TARGET_CHAR_BIT)
#define TARGET_CHAR_BIT 8
#endif

/* * If we picked up a copy of CHAR_BIT from a configuration file
   (which may get it by including <limits.h>) then use it to set
   the number of bits in a host char.  If not, use the same size
   as the target.  */

#if defined (CHAR_BIT)
#define HOST_CHAR_BIT CHAR_BIT
#else
#define HOST_CHAR_BIT TARGET_CHAR_BIT
#endif

/* In findvar.c.  */

extern LONGEST extract_signed_integer (const gdb_byte *, int,
				       enum bfd_endian);

extern ULONGEST extract_unsigned_integer (const gdb_byte *, int,
					  enum bfd_endian);

extern int extract_long_unsigned_integer (const gdb_byte *, int,
					  enum bfd_endian, LONGEST *);

extern CORE_ADDR extract_typed_address (const gdb_byte *buf,
					struct type *type);

extern void store_signed_integer (gdb_byte *, int,
				  enum bfd_endian, LONGEST);

extern void store_unsigned_integer (gdb_byte *, int,
				    enum bfd_endian, ULONGEST);

extern void store_typed_address (gdb_byte *buf, struct type *type,
				 CORE_ADDR addr);


/* From valops.c */

extern int watchdog;

/* Hooks for alternate command interfaces.  */

/* * The name of the interpreter if specified on the command line.  */
extern char *interpreter_p;

/* If a given interpreter matches INTERPRETER_P then it should update
   deprecated_init_ui_hook with the per-interpreter implementation.  */
/* FIXME: deprecated_init_ui_hook should be moved here.  */

struct target_waitstatus;
struct cmd_list_element;

extern void (*deprecated_pre_add_symbol_hook) (const char *);
extern void (*deprecated_post_add_symbol_hook) (void);
extern void (*selected_frame_level_changed_hook) (int);
extern int (*deprecated_ui_loop_hook) (int signo);
extern void (*deprecated_init_ui_hook) (char *argv0);
extern void (*deprecated_show_load_progress) (const char *section,
					      unsigned long section_sent, 
					      unsigned long section_size, 
					      unsigned long total_sent, 
					      unsigned long total_size);
extern void (*deprecated_print_frame_info_listing_hook) (struct symtab * s,
							 int line,
							 int stopline,
							 int noerror);
extern int (*deprecated_query_hook) (const char *, va_list)
     ATTRIBUTE_FPTR_PRINTF(1,0);
extern void (*deprecated_warning_hook) (const char *, va_list)
     ATTRIBUTE_FPTR_PRINTF(1,0);
extern void (*deprecated_interactive_hook) (void);
extern void (*deprecated_readline_begin_hook) (char *, ...)
     ATTRIBUTE_FPTR_PRINTF_1;
extern char *(*deprecated_readline_hook) (const char *);
extern void (*deprecated_readline_end_hook) (void);
extern void (*deprecated_register_changed_hook) (int regno);
extern void (*deprecated_context_hook) (int);
extern ptid_t (*deprecated_target_wait_hook) (ptid_t ptid,
					      struct target_waitstatus *status,
					      int options);

extern void (*deprecated_attach_hook) (void);
extern void (*deprecated_detach_hook) (void);
extern void (*deprecated_call_command_hook) (struct cmd_list_element * c,
					     char *cmd, int from_tty);

extern int (*deprecated_ui_load_progress_hook) (const char *section,
						unsigned long num);

/* If this definition isn't overridden by the header files, assume
   that isatty and fileno exist on this system.  */
#ifndef ISATTY
#define ISATTY(FP)	(isatty (fileno (FP)))
#endif

/* * A width that can achieve a better legibility for GDB MI mode.  */
#define GDB_MI_MSG_WIDTH  80

/* From progspace.c */

extern void initialize_progspace (void);
extern void initialize_inferiors (void);

/* * Special block numbers */

enum block_enum
{
  GLOBAL_BLOCK = 0,
  STATIC_BLOCK = 1,
  FIRST_LOCAL_BLOCK = 2
};

#include "utils.h"

#endif /* #ifndef DEFS_H */
