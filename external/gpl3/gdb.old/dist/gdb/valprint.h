/* Declarations for value printing routines for GDB, the GNU debugger.

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

#ifndef VALPRINT_H
#define VALPRINT_H

/* This is used to pass formatting options to various value-printing
   functions.  */
struct value_print_options
{
  /* Pretty-formatting control.  */
  enum val_prettyformat prettyformat;

  /* Controls pretty formatting of arrays.  */
  int prettyformat_arrays;

  /* Controls pretty formatting of structures.  */
  int prettyformat_structs;

  /* Controls printing of virtual tables.  */
  int vtblprint;

  /* Controls printing of nested unions.  */
  int unionprint;

  /* Controls printing of addresses.  */
  int addressprint;

  /* Controls looking up an object's derived type using what we find
     in its vtables.  */
  int objectprint;

  /* Maximum number of chars to print for a string pointer value or vector
     contents, or UINT_MAX for no limit.  Note that "set print elements 0"
     stores UINT_MAX in print_max, which displays in a show command as
     "unlimited".  */
  unsigned int print_max;

  /* Print repeat counts if there are more than this many repetitions
     of an element in an array.  */
  unsigned int repeat_count_threshold;

  /* The global output format letter.  */
  int output_format;

  /* The current format letter.  This is set locally for a given call,
     e.g. when the user passes a format to "print".  */
  int format;

  /* Stop printing at null character?  */
  int stop_print_at_null;

  /* True if we should print the index of each element when printing
     an array.  */
  int print_array_indexes;

  /* If nonzero, then dereference references, otherwise just print
     them like pointers.  */
  int deref_ref;

  /* If nonzero, print static fields.  */
  int static_field_print;

  /* If nonzero, print static fields for Pascal.  FIXME: C++ has a
     flag, why not share with Pascal too?  */
  int pascal_static_field_print;

  /* If non-zero don't do Python pretty-printing.  */
  int raw;

  /* If nonzero, print the value in "summary" form.
     If raw and summary are both non-zero, don't print non-scalar values
     ("..." is printed instead).  */
  int summary;

  /* If nonzero, when printing a pointer, print the symbol to which it
     points, if any.  */
  int symbol_print;
};

/* The global print options set by the user.  In general this should
   not be directly accessed, except by set/show commands.  Ordinary
   code should call get_user_print_options instead.  */
extern struct value_print_options user_print_options;

/* Initialize *OPTS to be a copy of the user print options.  */
extern void get_user_print_options (struct value_print_options *opts);

/* Initialize *OPTS to be a copy of the user print options, but with
   pretty-formatting disabled.  */
extern void get_no_prettyformat_print_options (struct value_print_options *);

/* Initialize *OPTS to be a copy of the user print options, but using
   FORMAT as the formatting option.  */
extern void get_formatted_print_options (struct value_print_options *opts,
					 char format);

extern void maybe_print_array_index (struct type *index_type, LONGEST index,
                                     struct ui_file *stream,
				     const struct value_print_options *);

extern void val_print_array_elements (struct type *, LONGEST,
				      CORE_ADDR, struct ui_file *, int,
				      struct value *,
				      const struct value_print_options *,
				      unsigned int);

extern void val_print_type_code_int (struct type *, const gdb_byte *,
				     struct ui_file *);

extern void val_print_scalar_formatted (struct type *,
					LONGEST,
					struct value *,
					const struct value_print_options *,
					int,
					struct ui_file *);

extern void print_binary_chars (struct ui_file *, const gdb_byte *,
				unsigned int, enum bfd_endian);

extern void print_octal_chars (struct ui_file *, const gdb_byte *,
			       unsigned int, enum bfd_endian);

extern void print_decimal_chars (struct ui_file *, const gdb_byte *,
				 unsigned int, enum bfd_endian);

extern void print_hex_chars (struct ui_file *, const gdb_byte *,
			     unsigned int, enum bfd_endian);

extern void print_char_chars (struct ui_file *, struct type *,
			      const gdb_byte *, unsigned int, enum bfd_endian);

extern void print_function_pointer_address (const struct value_print_options *options,
					    struct gdbarch *gdbarch,
					    CORE_ADDR address,
					    struct ui_file *stream);

extern int read_string (CORE_ADDR addr, int len, int width,
			unsigned int fetchlimit,
			enum bfd_endian byte_order, gdb_byte **buffer,
			int *bytes_read);

extern void val_print_optimized_out (const struct value *val,
				     struct ui_file *stream);

/* Prints "<not saved>" to STREAM.  */
extern void val_print_not_saved (struct ui_file *stream);

extern void val_print_unavailable (struct ui_file *stream);

extern void val_print_invalid_address (struct ui_file *stream);

/* An instance of this is passed to generic_val_print and describes
   some language-specific ways to print things.  */

struct generic_val_print_decorations
{
  /* Printing complex numbers: what to print before, between the
     elements, and after.  */

  const char *complex_prefix;
  const char *complex_infix;
  const char *complex_suffix;

  /* Boolean true and false.  */

  const char *true_name;
  const char *false_name;

  /* What to print when we see TYPE_CODE_VOID.  */

  const char *void_name;

  /* Array start and end strings.  */
  const char *array_start;
  const char *array_end;
};


extern void generic_val_print (struct type *type,
			       int embedded_offset, CORE_ADDR address,
			       struct ui_file *stream, int recurse,
			       struct value *original_value,
			       const struct value_print_options *options,
			       const struct generic_val_print_decorations *);

extern void generic_emit_char (int c, struct type *type, struct ui_file *stream,
			       int quoter, const char *encoding);

extern void generic_printstr (struct ui_file *stream, struct type *type, 
			      const gdb_byte *string, unsigned int length, 
			      const char *encoding, int force_ellipses,
			      int quote_char, int c_style_terminator,
			      const struct value_print_options *options);

/* Run the "output" command.  ARGS and FROM_TTY are the usual
   arguments passed to all command implementations, except ARGS is
   const.  */

extern void output_command_const (const char *args, int from_tty);

extern int val_print_scalar_type_p (struct type *type);

struct format_data
  {
    int count;
    char format;
    char size;

    /* True if the value should be printed raw -- that is, bypassing
       python-based formatters.  */
    unsigned char raw;
  };

extern void print_command_parse_format (const char **expp, const char *cmdname,
					struct format_data *fmtp);
extern void print_value (struct value *val, const struct format_data *fmtp);

#endif
