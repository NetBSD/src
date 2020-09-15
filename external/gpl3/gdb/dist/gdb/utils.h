/* *INDENT-OFF* */ /* ATTRIBUTE_PRINTF confuses indent, avoid running it
		      for now.  */
/* I/O, string, cleanup, and other random utilities for GDB.
   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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

#ifndef UTILS_H
#define UTILS_H

#include "exceptions.h"
#include "gdbsupport/array-view.h"
#include "gdbsupport/scoped_restore.h"
#include <chrono>

#ifdef HAVE_LIBXXHASH
#include <xxhash.h>
#endif

struct completion_match_for_lcd;
class compiled_regex;

/* String utilities.  */

extern bool sevenbit_strings;

/* Modes of operation for strncmp_iw_with_mode.  */

enum class strncmp_iw_mode
{
/* Do a strcmp() type operation on STRING1 and STRING2, ignoring any
   differences in whitespace.  Returns 0 if they match, non-zero if
   they don't (slightly different than strcmp()'s range of return
   values).  */
  NORMAL,

  /* Like NORMAL, but also apply the strcmp_iw hack.  I.e.,
     string1=="FOO(PARAMS)" matches string2=="FOO".  */
  MATCH_PARAMS,
};

/* Helper for strcmp_iw and strncmp_iw.  Exported so that languages
   can implement both NORMAL and MATCH_PARAMS variants in a single
   function and defer part of the work to strncmp_iw_with_mode.

   LANGUAGE is used to implement some context-sensitive
   language-specific comparisons.  For example, for C++,
   "string1=operator()" should not match "string2=operator" even in
   MATCH_PARAMS mode.

   MATCH_FOR_LCD is passed down so that the function can mark parts of
   the symbol name as ignored for completion matching purposes (e.g.,
   to handle abi tags).  */
extern int strncmp_iw_with_mode
  (const char *string1, const char *string2, size_t string2_len,
   strncmp_iw_mode mode, enum language language,
   completion_match_for_lcd *match_for_lcd = NULL);

/* Do a strncmp() type operation on STRING1 and STRING2, ignoring any
   differences in whitespace.  STRING2_LEN is STRING2's length.
   Returns 0 if STRING1 matches STRING2_LEN characters of STRING2,
   non-zero otherwise (slightly different than strncmp()'s range of
   return values).  Note: passes language_minimal to
   strncmp_iw_with_mode, and should therefore be avoided if a more
   suitable language is available.  */
extern int strncmp_iw (const char *string1, const char *string2,
		       size_t string2_len);

/* Do a strcmp() type operation on STRING1 and STRING2, ignoring any
   differences in whitespace.  Returns 0 if they match, non-zero if
   they don't (slightly different than strcmp()'s range of return
   values).

   As an extra hack, string1=="FOO(ARGS)" matches string2=="FOO".
   This "feature" is useful when searching for matching C++ function
   names (such as if the user types 'break FOO', where FOO is a
   mangled C++ function).

   Note: passes language_minimal to strncmp_iw_with_mode, and should
   therefore be avoided if a more suitable language is available.  */
extern int strcmp_iw (const char *string1, const char *string2);

extern int strcmp_iw_ordered (const char *, const char *);

/* Return true if the strings are equal.  */

extern bool streq (const char *, const char *);

/* A variant of streq that is suitable for use as an htab
   callback.  */

extern int streq_hash (const void *, const void *);

extern int subset_compare (const char *, const char *);

/* Compare C strings for std::sort.  */

static inline bool
compare_cstrings (const char *str1, const char *str2)
{
  return strcmp (str1, str2) < 0;
}

/* A wrapper for bfd_errmsg to produce a more helpful error message
   in the case of bfd_error_file_ambiguously recognized.
   MATCHING, if non-NULL, is the corresponding argument to
   bfd_check_format_matches, and will be freed.  */

extern std::string gdb_bfd_errmsg (bfd_error_type error_tag, char **matching);

/* Reset the prompt_for_continue clock.  */
void reset_prompt_for_continue_wait_time (void);
/* Return the time spent in prompt_for_continue.  */
std::chrono::steady_clock::duration get_prompt_for_continue_wait_time ();

/* Parsing utilities.  */

extern int parse_pid_to_attach (const char *args);

extern int parse_escape (struct gdbarch *, const char **);

/* A wrapper for an array of char* that was allocated in the way that
   'buildargv' does, and should be freed with 'freeargv'.  */

class gdb_argv
{
public:

  /* A constructor that initializes to NULL.  */

  gdb_argv ()
    : m_argv (NULL)
  {
  }

  /* A constructor that calls buildargv on STR.  STR may be NULL, in
     which case this object is initialized with a NULL array.  */

  explicit gdb_argv (const char *str)
    : m_argv (NULL)
  {
    reset (str);
  }

  /* A constructor that takes ownership of an existing array.  */

  explicit gdb_argv (char **array)
    : m_argv (array)
  {
  }

  gdb_argv (const gdb_argv &) = delete;
  gdb_argv &operator= (const gdb_argv &) = delete;

  ~gdb_argv ()
  {
    freeargv (m_argv);
  }

  /* Call buildargv on STR, storing the result in this object.  Any
     previous state is freed.  STR may be NULL, in which case this
     object is reset with a NULL array.  If buildargv fails due to
     out-of-memory, call malloc_failure.  Therefore, the value is
     guaranteed to be non-NULL, unless the parameter itself is
     NULL.  */

  void reset (const char *str);

  /* Return the underlying array.  */

  char **get ()
  {
    return m_argv;
  }

  /* Return the underlying array, transferring ownership to the
     caller.  */

  ATTRIBUTE_UNUSED_RESULT char **release ()
  {
    char **result = m_argv;
    m_argv = NULL;
    return result;
  }

  /* Return the number of items in the array.  */

  int count () const
  {
    return countargv (m_argv);
  }

  /* Index into the array.  */

  char *operator[] (int arg)
  {
    gdb_assert (m_argv != NULL);
    return m_argv[arg];
  }

  /* Return the arguments array as an array view.  */

  gdb::array_view<char *> as_array_view ()
  {
    return gdb::array_view<char *> (this->get (), this->count ());
  }

  /* The iterator type.  */

  typedef char **iterator;

  /* Return an iterator pointing to the start of the array.  */

  iterator begin ()
  {
    return m_argv;
  }

  /* Return an iterator pointing to the end of the array.  */

  iterator end ()
  {
    return m_argv + count ();
  }

  bool operator!= (std::nullptr_t)
  {
    return m_argv != NULL;
  }

  bool operator== (std::nullptr_t)
  {
    return m_argv == NULL;
  }

private:

  /* The wrapped array.  */

  char **m_argv;
};


/* Cleanup utilities.  */

/* A deleter for a hash table.  */
struct htab_deleter
{
  void operator() (htab *ptr) const
  {
    htab_delete (ptr);
  }
};

/* A unique_ptr wrapper for htab_t.  */
typedef std::unique_ptr<htab, htab_deleter> htab_up;

extern void init_page_info (void);

/* Temporarily set BATCH_FLAG and the associated unlimited terminal size.
   Restore when destroyed.  */

struct set_batch_flag_and_restore_page_info
{
public:

  set_batch_flag_and_restore_page_info ();
  ~set_batch_flag_and_restore_page_info ();

  DISABLE_COPY_AND_ASSIGN (set_batch_flag_and_restore_page_info);

private:

  /* Note that this doesn't use scoped_restore, because it's important
     to control the ordering of operations in the destruction, and it
     was simpler to avoid introducing a new ad hoc class.  */
  unsigned m_save_lines_per_page;
  unsigned m_save_chars_per_line;
  int m_save_batch_flag;
};


/* Path utilities.  */

extern int gdb_filename_fnmatch (const char *pattern, const char *string,
				 int flags);

extern void substitute_path_component (char **stringp, const char *from,
				       const char *to);

std::string ldirname (const char *filename);

extern int count_path_elements (const char *path);

extern const char *strip_leading_path_elements (const char *path, int n);

/* GDB output, ui_file utilities.  */

struct ui_file;

extern int query (const char *, ...) ATTRIBUTE_PRINTF (1, 2);
extern int nquery (const char *, ...) ATTRIBUTE_PRINTF (1, 2);
extern int yquery (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

extern void begin_line (void);

extern void wrap_here (const char *);

extern void reinitialize_more_filter (void);

extern bool pagination_enabled;

extern struct ui_file **current_ui_gdb_stdout_ptr (void);
extern struct ui_file **current_ui_gdb_stdin_ptr (void);
extern struct ui_file **current_ui_gdb_stderr_ptr (void);
extern struct ui_file **current_ui_gdb_stdlog_ptr (void);

/* Flush STREAM.  This is a wrapper for ui_file_flush that also
   flushes any output pending from uses of the *_filtered output
   functions; that output is kept in a special buffer so that
   pagination and styling are handled properly.  */
extern void gdb_flush (struct ui_file *);

/* The current top level's ui_file streams.  */

/* Normal results */
#define gdb_stdout (*current_ui_gdb_stdout_ptr ())
/* Input stream */
#define gdb_stdin (*current_ui_gdb_stdin_ptr ())
/* Serious error notifications */
#define gdb_stderr (*current_ui_gdb_stderr_ptr ())
/* Log/debug/trace messages that should bypass normal stdout/stderr
   filtering.  For moment, always call this stream using
   *_unfiltered.  In the very near future that restriction shall be
   removed - either call shall be unfiltered.  (cagney 1999-06-13).  */
#define gdb_stdlog (*current_ui_gdb_stdlog_ptr ())

/* Truly global ui_file streams.  These are all defined in main.c.  */

/* Target output that should bypass normal stdout/stderr filtering.
   For moment, always call this stream using *_unfiltered.  In the
   very near future that restriction shall be removed - either call
   shall be unfiltered.  (cagney 1999-07-02).  */
extern struct ui_file *gdb_stdtarg;
extern struct ui_file *gdb_stdtargerr;
extern struct ui_file *gdb_stdtargin;

/* Set the screen dimensions to WIDTH and HEIGHT.  */

extern void set_screen_width_and_height (int width, int height);

/* More generic printf like operations.  Filtered versions may return
   non-locally on error.  As an extension over plain printf, these
   support some GDB-specific format specifiers.  Particularly useful
   here are the styling formatters: '%p[', '%p]' and '%ps'.  See
   ui_out::message for details.  */

extern void fputs_filtered (const char *, struct ui_file *);

extern void fputs_unfiltered (const char *, struct ui_file *);

extern int fputc_filtered (int c, struct ui_file *);

extern int fputc_unfiltered (int c, struct ui_file *);

extern int putchar_filtered (int c);

extern int putchar_unfiltered (int c);

extern void puts_filtered (const char *);

extern void puts_unfiltered (const char *);

extern void puts_filtered_tabular (char *string, int width, int right);

extern void puts_debug (char *prefix, char *string, char *suffix);

extern void vprintf_filtered (const char *, va_list) ATTRIBUTE_PRINTF (1, 0);

extern void vfprintf_filtered (struct ui_file *, const char *, va_list)
  ATTRIBUTE_PRINTF (2, 0);

extern void fprintf_filtered (struct ui_file *, const char *, ...)
  ATTRIBUTE_PRINTF (2, 3);

extern void fprintfi_filtered (int, struct ui_file *, const char *, ...)
  ATTRIBUTE_PRINTF (3, 4);

extern void printf_filtered (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

extern void printfi_filtered (int, const char *, ...) ATTRIBUTE_PRINTF (2, 3);

extern void vprintf_unfiltered (const char *, va_list) ATTRIBUTE_PRINTF (1, 0);

extern void vfprintf_unfiltered (struct ui_file *, const char *, va_list)
  ATTRIBUTE_PRINTF (2, 0);

extern void fprintf_unfiltered (struct ui_file *, const char *, ...)
  ATTRIBUTE_PRINTF (2, 3);

extern void printf_unfiltered (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

extern void print_spaces (int, struct ui_file *);

extern void print_spaces_filtered (int, struct ui_file *);

extern char *n_spaces (int);

extern void fputstr_filtered (const char *str, int quotr,
			      struct ui_file * stream);

extern void fputstr_unfiltered (const char *str, int quotr,
				struct ui_file * stream);

extern void fputstrn_filtered (const char *str, int n, int quotr,
			       struct ui_file * stream);

typedef int (*do_fputc_ftype) (int c, ui_file *stream);

extern void fputstrn_unfiltered (const char *str, int n, int quotr,
				 do_fputc_ftype do_fputc,
				 struct ui_file * stream);

/* Return nonzero if filtered printing is initialized.  */
extern int filtered_printing_initialized (void);

/* Like fprintf_filtered, but styles the output according to STYLE,
   when appropriate.  */

extern void fprintf_styled (struct ui_file *stream,
			    const ui_file_style &style,
			    const char *fmt,
			    ...)
  ATTRIBUTE_PRINTF (3, 4);

extern void vfprintf_styled (struct ui_file *stream,
			     const ui_file_style &style,
			     const char *fmt,
			     va_list args)
  ATTRIBUTE_PRINTF (3, 0);

/* Like vfprintf_styled, but do not process gdb-specific format
   specifiers.  */
extern void vfprintf_styled_no_gdbfmt (struct ui_file *stream,
				       const ui_file_style &style,
				       bool filter,
				       const char *fmt, va_list args)
  ATTRIBUTE_PRINTF (4, 0);

/* Like fputs_filtered, but styles the output according to STYLE, when
   appropriate.  */

extern void fputs_styled (const char *linebuffer,
			  const ui_file_style &style,
			  struct ui_file *stream);

/* Unfiltered variant of fputs_styled.  */

extern void fputs_styled_unfiltered (const char *linebuffer,
				     const ui_file_style &style,
				     struct ui_file *stream);

/* Like fputs_styled, but uses highlight_style to highlight the
   parts of STR that match HIGHLIGHT.  */

extern void fputs_highlighted (const char *str, const compiled_regex &highlight,
			       struct ui_file *stream);

/* Reset the terminal style to the default, if needed.  */

extern void reset_terminal_style (struct ui_file *stream);

/* Display the host ADDR on STREAM formatted as ``0x%x''.  */
extern void gdb_print_host_address_1 (const void *addr, struct ui_file *stream);

/* Wrapper that avoids adding a pointless cast to all callers.  */
#define gdb_print_host_address(ADDR, STREAM) \
  gdb_print_host_address_1 ((const void *) ADDR, STREAM)

/* Return the address only having significant bits.  */
extern CORE_ADDR address_significant (gdbarch *gdbarch, CORE_ADDR addr);

/* Convert CORE_ADDR to string in platform-specific manner.
   This is usually formatted similar to 0x%lx.  */
extern const char *paddress (struct gdbarch *gdbarch, CORE_ADDR addr);

/* Return a string representation in hexadecimal notation of ADDRESS,
   which is suitable for printing.  */

extern const char *print_core_address (struct gdbarch *gdbarch,
				       CORE_ADDR address);

/* Callback hash_f and eq_f for htab_create_alloc or htab_create_alloc_ex.  */
extern hashval_t core_addr_hash (const void *ap);
extern int core_addr_eq (const void *ap, const void *bp);

extern CORE_ADDR string_to_core_addr (const char *my_string);

extern void fprintf_symbol_filtered (struct ui_file *, const char *,
				     enum language, int);

extern void throw_perror_with_name (enum errors errcode, const char *string)
  ATTRIBUTE_NORETURN;

extern void perror_warning_with_name (const char *string);

extern void print_sys_errmsg (const char *, int);

/* Warnings and error messages.  */

extern void (*deprecated_error_begin_hook) (void);

/* Message to be printed before the warning message, when a warning occurs.  */

extern const char *warning_pre_print;

extern void error_stream (const string_file &) ATTRIBUTE_NORETURN;

extern void demangler_vwarning (const char *file, int line,
			       const char *, va_list ap)
     ATTRIBUTE_PRINTF (3, 0);

extern void demangler_warning (const char *file, int line,
			      const char *, ...) ATTRIBUTE_PRINTF (3, 4);


/* Misc. utilities.  */

/* Allocation and deallocation functions for the libiberty hash table
   which use obstacks.  */
void *hashtab_obstack_allocate (void *data, size_t size, size_t count);
void dummy_obstack_deallocate (void *object, void *data);

#ifdef HAVE_WAITPID
extern pid_t wait_to_die_with_timeout (pid_t pid, int *status, int timeout);
#endif

extern int myread (int, char *, int);

/* Resource limits used by getrlimit and setrlimit.  */

enum resource_limit_kind
  {
    LIMIT_CUR,
    LIMIT_MAX
  };

/* Check whether GDB will be able to dump core using the dump_core
   function.  Returns zero if GDB cannot or should not dump core.
   If LIMIT_KIND is LIMIT_CUR the user's soft limit will be respected.
   If LIMIT_KIND is LIMIT_MAX only the hard limit will be respected.  */

extern int can_dump_core (enum resource_limit_kind limit_kind);

/* Print a warning that we cannot dump core.  */

extern void warn_cant_dump_core (const char *reason);

/* Dump core trying to increase the core soft limit to hard limit
   first.  */

extern void dump_core (void);

/* Copy NBITS bits from SOURCE to DEST starting at the given bit
   offsets.  Use the bit order as specified by BITS_BIG_ENDIAN.
   Source and destination buffers must not overlap.  */

extern void copy_bitwise (gdb_byte *dest, ULONGEST dest_offset,
			  const gdb_byte *source, ULONGEST source_offset,
			  ULONGEST nbits, int bits_big_endian);

/* A fast hashing function.  This can be used to hash data in a fast way
   when the length is known.  If no fast hashing library is available, falls
   back to iterative_hash from libiberty.  START_VALUE can be set to
   continue hashing from a previous value.  */

static inline unsigned int
fast_hash (const void *ptr, size_t len, unsigned int start_value = 0)
{
#ifdef HAVE_LIBXXHASH
  return XXH64 (ptr, len, start_value);
#else
  return iterative_hash (ptr, len, start_value);
#endif
}

#endif /* UTILS_H */
