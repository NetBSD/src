/* Output generating routines for GDB.
   Copyright 1999, 2000 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.
   Written by Fernando Nasser for Cygnus.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdb_string.h"
#include "expression.h"		/* For language.h */
#include "language.h"
#include "ui-out.h"

/* Convenience macro for allocting typesafe memory. */

#undef XMALLOC
#define XMALLOC(TYPE) (TYPE*) xmalloc (sizeof (TYPE))

/* table header structures */

struct ui_out_hdr
  {
    int colno;
    int width;
    int alignment;
    char *colhdr;
    struct ui_out_hdr *next;
  };

/* The ui_out structure */
/* Any change here requires a corresponding one in the initialization
   of the default uiout, which is statically initialized */

struct ui_out
  {
    int flags;
    /* specific implementation of ui-out */
    struct ui_out_impl *impl;
    struct ui_out_data *data;

    /* if on, a table is being generated */
    int table_flag;

    /* if on, the body of a table is being generated */
    int body_flag;

    /* number of table columns (as specified in the table_begin call) */
    int table_columns;

    /* strinf identifying the table (as specified in the table_begin call) */
    char *table_id;

    /* if on, a list is being generated.  The value is the level of nesting */
    int list_flag;

    /* we count each field; the first element is for non-list fields */
    int field_count[5];

    /* points to the first header (if any) */
    struct ui_out_hdr *headerfirst;

    /* points to the last header (if any) */
    struct ui_out_hdr *headerlast;

    /* points to header of next column to format */
    struct ui_out_hdr *headercurr;

  };

/* These are the default implementation functions */

static void default_table_begin (struct ui_out *uiout, int nbrofcols,
				 char *tblid);
static void default_table_body (struct ui_out *uiout);
static void default_table_end (struct ui_out *uiout);
static void default_table_header (struct ui_out *uiout, int width,
				  enum ui_align alig, char *colhdr);
static void default_list_begin (struct ui_out *uiout, int list_flag,
				char *lstid);
static void default_list_end (struct ui_out *uiout, int list_flag);
static void default_field_int (struct ui_out *uiout, int fldno, int width,
			       enum ui_align alig, char *fldname, int value);
static void default_field_skip (struct ui_out *uiout, int fldno, int width,
				enum ui_align alig, char *fldname);
static void default_field_string (struct ui_out *uiout, int fldno, int width,
				  enum ui_align align, char *fldname,
				  const char *string);
static void default_field_fmt (struct ui_out *uiout, int fldno,
			       int width, enum ui_align align,
			       char *fldname, char *format, va_list args);
static void default_spaces (struct ui_out *uiout, int numspaces);
static void default_text (struct ui_out *uiout, char *string);
static void default_message (struct ui_out *uiout, int verbosity, char *format,
			     va_list args);
static void default_wrap_hint (struct ui_out *uiout, char *identstring);
static void default_flush (struct ui_out *uiout);

/* This is the default ui-out implementation functions vector */

struct ui_out_impl default_ui_out_impl =
{
  default_table_begin,
  default_table_body,
  default_table_end,
  default_table_header,
  default_list_begin,
  default_list_end,
  default_field_int,
  default_field_skip,
  default_field_string,
  default_field_fmt,
  default_spaces,
  default_text,
  default_message,
  default_wrap_hint,
  default_flush
};

/* The default ui_out */

struct ui_out def_uiout =
{
  0,				/* flags */
  &default_ui_out_impl,		/* impl */
};

/* Pointer to current ui_out */
/* FIXME: This should not be a global, but something passed down from main.c
   or top.c */

struct ui_out *uiout = &def_uiout;

/* These are the interfaces to implementation functions */

static void uo_table_begin (struct ui_out *uiout, int nbrofcols, char *tblid);
static void uo_table_body (struct ui_out *uiout);
static void uo_table_end (struct ui_out *uiout);
static void uo_table_header (struct ui_out *uiout, int width,
			     enum ui_align align, char *colhdr);
static void uo_list_begin (struct ui_out *uiout, int list_flag, char *lstid);
static void uo_list_end (struct ui_out *uiout, int list_flag);
static void uo_field_int (struct ui_out *uiout, int fldno, int width,
			  enum ui_align align, char *fldname, int value);
static void uo_field_skip (struct ui_out *uiout, int fldno, int width,
			   enum ui_align align, char *fldname);
static void uo_field_string (struct ui_out *uiout, int fldno, int width,
			  enum ui_align align, char *fldname, const char *string);
static void uo_field_fmt (struct ui_out *uiout, int fldno, int width,
			  enum ui_align align, char *fldname,
			  char *format, va_list args);
static void uo_spaces (struct ui_out *uiout, int numspaces);
static void uo_text (struct ui_out *uiout, char *string);
static void uo_message (struct ui_out *uiout, int verbosity,
			char *format, va_list args);
static void uo_wrap_hint (struct ui_out *uiout, char *identstring);
static void uo_flush (struct ui_out *uiout);

/* Prototypes for local functions */

extern void _initialize_ui_out (void);
static void append_header_to_list (struct ui_out *uiout, int width, int alignment, char *colhdr);
static int get_curr_header (struct ui_out *uiout, int *colno, int *width,
			    int *alignment, char **colhdr);
static void clear_header_list (struct ui_out *uiout);
static void verify_field_proper_position (struct ui_out *uiout);
static void verify_field_alignment (struct ui_out *uiout, int fldno, int *width, int *alignment);

static void init_ui_out_state (struct ui_out *uiout);

/* exported functions (ui_out API) */

/* Mark beginning of a table */

void
ui_out_table_begin (uiout, nbrofcols, tblid)
     struct ui_out *uiout;
     int nbrofcols;
     char *tblid;
{
  if (uiout->table_flag)
    internal_error ("gdb/ui_out.c: tables cannot be nested; table_begin found before \
previous table_end.");

  uiout->table_flag = 1;
  uiout->table_columns = nbrofcols;
  if (tblid != NULL)
    uiout->table_id = xstrdup (tblid);
  else
    uiout->table_id = NULL;
  clear_header_list (uiout);

  uo_table_begin (uiout, nbrofcols, uiout->table_id);
}

void
ui_out_table_body (uiout)
     struct ui_out *uiout;
{
  if (!uiout->table_flag)
    internal_error ("gdb/ui_out.c: table_body outside a table is not valid; it must be \
after a table_begin and before a table_end.");
  if (uiout->body_flag)
    internal_error ("gdb/ui_out.c: extra table_body call not allowed; there must be \
only one table_body after a table_begin and before a table_end.");
  if (uiout->headercurr->colno != uiout->table_columns)
    internal_error ("gdb/ui_out.c: number of headers differ from number of table \
columns.");

  uiout->body_flag = 1;
  uiout->headercurr = uiout->headerfirst;

  uo_table_body (uiout);
}

void
ui_out_table_end (uiout)
     struct ui_out *uiout;
{
  if (!uiout->table_flag)
    internal_error ("gdb/ui_out.c: misplaced table_end or missing table_begin.");

  uiout->body_flag = 0;
  uiout->table_flag = 0;

  uo_table_end (uiout);

  if (uiout->table_id)
    free (uiout->table_id);
  clear_header_list (uiout);
}

void
ui_out_table_header (uiout, width, alignment, colhdr)
     struct ui_out *uiout;
     int width;
     enum ui_align alignment;
     char *colhdr;
{
  if (!uiout->table_flag || uiout->body_flag)
    internal_error ("ui_out: table header must be specified after table_begin \
and before table_body.");

  append_header_to_list (uiout, width, alignment, colhdr);

  uo_table_header (uiout, width, alignment, colhdr);
}

void
ui_out_list_begin (uiout, lstid)
     struct ui_out *uiout;
     char *lstid;
{
  if (uiout->table_flag && !uiout->body_flag)
    internal_error ("ui_out: table header or table_body expected; lists must be \
specified after table_body.");
  if (uiout->list_flag >= 4)
    internal_error ("ui_out: list depth exceeded; only 4 levels of lists can be \
nested.");

  uiout->list_flag++;
  uiout->field_count[uiout->list_flag] = 0;
  if (uiout->table_flag && (uiout->list_flag == 1))
    uiout->headercurr = uiout->headerfirst;

  uo_list_begin (uiout, uiout->list_flag, lstid);
}

void
ui_out_list_end (uiout)
     struct ui_out *uiout;
{
  if (!uiout->list_flag)
    internal_error ("ui_out: misplaced list_end; there is no list to be closed.");

  uo_list_end (uiout, uiout->list_flag);

  uiout->list_flag--;
}

void
ui_out_field_int (uiout, fldname, value)
     struct ui_out *uiout;
     char *fldname;
     int value;
{
  int fldno;
  int width;
  int align;

  verify_field_proper_position (uiout);

  uiout->field_count[uiout->list_flag] += 1;
  fldno = uiout->field_count[uiout->list_flag];

  verify_field_alignment (uiout, fldno, &width, &align);

  uo_field_int (uiout, fldno, width, align, fldname, value);
}

void
ui_out_field_core_addr (uiout, fldname, address)
     struct ui_out *uiout;
     char *fldname;
     CORE_ADDR address;
{
  char addstr[20];

  /* FIXME-32x64: need a print_address_numeric with field width */
  /* print_address_numeric (address, 1, local_stream); */
  strcpy (addstr, local_hex_string_custom ((unsigned long) address, "08l"));

  ui_out_field_string (uiout, fldname, addstr);
}

void
ui_out_field_stream (uiout, fldname, buf)
     struct ui_out *uiout;
     char *fldname;
     struct ui_stream *buf;
{
  long length;
  char *buffer = ui_file_xstrdup (buf->stream, &length);
  struct cleanup *old_cleanup = make_cleanup (free, buffer);
  if (length > 0)
    ui_out_field_string (uiout, fldname, buffer);
  else
    ui_out_field_skip (uiout, fldname);
  ui_file_rewind (buf->stream);
  do_cleanups (old_cleanup);
}

/* used to ommit a field */

void
ui_out_field_skip (uiout, fldname)
     struct ui_out *uiout;
     char *fldname;
{
  int fldno;
  int width;
  int align;

  verify_field_proper_position (uiout);

  uiout->field_count[uiout->list_flag] += 1;
  fldno = uiout->field_count[uiout->list_flag];

  verify_field_alignment (uiout, fldno, &width, &align);

  uo_field_skip (uiout, fldno, width, align, fldname);
}

void
ui_out_field_string (struct ui_out *uiout,
		     char *fldname,
		     const char *string)
{
  int fldno;
  int width;
  int align;

  verify_field_proper_position (uiout);

  uiout->field_count[uiout->list_flag] += 1;
  fldno = uiout->field_count[uiout->list_flag];

  verify_field_alignment (uiout, fldno, &width, &align);

  uo_field_string (uiout, fldno, width, align, fldname, string);
}

/* VARARGS */
void
ui_out_field_fmt (struct ui_out *uiout, char *fldname, char *format,...)
{
  va_list args;
  int fldno;
  int width;
  int align;

  verify_field_proper_position (uiout);

  uiout->field_count[uiout->list_flag] += 1;
  fldno = uiout->field_count[uiout->list_flag];

  /* will not align, but has to call anyway */
  verify_field_alignment (uiout, fldno, &width, &align);

  va_start (args, format);

  uo_field_fmt (uiout, fldno, width, align, fldname, format, args);

  va_end (args);
}

void
ui_out_spaces (uiout, numspaces)
     struct ui_out *uiout;
     int numspaces;
{
  uo_spaces (uiout, numspaces);
}

void
ui_out_text (uiout, string)
     struct ui_out *uiout;
     char *string;
{
  uo_text (uiout, string);
}

void
ui_out_message (struct ui_out *uiout, int verbosity, char *format,...)
{
  va_list args;

  va_start (args, format);

  uo_message (uiout, verbosity, format, args);

  va_end (args);
}

struct ui_stream *
ui_out_stream_new (uiout)
     struct ui_out *uiout;
{
  struct ui_stream *tempbuf;

  tempbuf = XMALLOC (struct ui_stream);
  tempbuf->uiout = uiout;
  tempbuf->stream = mem_fileopen ();
  return tempbuf;
}

void
ui_out_stream_delete (buf)
     struct ui_stream *buf;
{
  ui_file_delete (buf->stream);
  free (buf);
}

static void
do_stream_delete (void *buf)
{
  ui_out_stream_delete (buf);
}

struct cleanup *
make_cleanup_ui_out_stream_delete (struct ui_stream *buf)
{
  return make_cleanup (do_stream_delete, buf);
}


void
ui_out_wrap_hint (uiout, identstring)
     struct ui_out *uiout;
     char *identstring;
{
  uo_wrap_hint (uiout, identstring);
}

void
ui_out_flush (uiout)
     struct ui_out *uiout;
{
  uo_flush (uiout);
}

/* set the flags specified by the mask given */
int
ui_out_set_flags (uiout, mask)
     struct ui_out *uiout;
     int mask;
{
  int oldflags = uiout->flags;

  uiout->flags |= mask;

  return oldflags;
}

/* clear the flags specified by the mask given */
int
ui_out_clear_flags (uiout, mask)
     struct ui_out *uiout;
     int mask;
{
  int oldflags = uiout->flags;

  uiout->flags &= ~mask;

  return oldflags;
}

/* test the flags against the mask given */
int
ui_out_test_flags (uiout, mask)
     struct ui_out *uiout;
     int mask;
{
  return (uiout->flags & mask);
}

/* obtain the current verbosity level (as stablished by the
   'set verbositylevel' command */

int
ui_out_get_verblvl (uiout)
     struct ui_out *uiout;
{
  /* FIXME: not implemented yet */
  return 0;
}

#if 0
void
ui_out_result_begin (uiout, class)
     struct ui_out *uiout;
     char *class;
{
}

void
ui_out_result_end (uiout)
     struct ui_out *uiout;
{
}

void
ui_out_info_begin (uiout, class)
     struct ui_out *uiout;
     char *class;
{
}

void
ui_out_info_end (uiout)
     struct ui_out *uiout;
{
}

void
ui_out_notify_begin (uiout, class)
     struct ui_out *uiout;
     char *class;
{
}

void
ui_out_notify_end (uiout)
     struct ui_out *uiout;
{
}

void
ui_out_error_begin (uiout, class)
     struct ui_out *uiout;
     char *class;
{
}

void
ui_out_error_end (uiout)
     struct ui_out *uiout;
{
}
#endif

#if 0
void
gdb_error (ui_out * uiout, int severity, char *format,...)
{
  va_list args;
}

void
gdb_query (uiout, qflags, qprompt)
     struct ui_out *uiout;
     int flags;
     char *qprompt;
{
}
#endif

/* default gdb-out hook functions */

static void
default_table_begin (uiout, nbrofcols, tblid)
     struct ui_out *uiout;
     int nbrofcols;
     char *tblid;
{
}

static void
default_table_body (uiout)
     struct ui_out *uiout;
{
}

static void
default_table_end (uiout)
     struct ui_out *uiout;
{
}

static void
default_table_header (uiout, width, alignment, colhdr)
     struct ui_out *uiout;
     int width;
     enum ui_align alignment;
     char *colhdr;
{
}

static void
default_list_begin (uiout, list_flag, lstid)
     struct ui_out *uiout;
     int list_flag;
     char *lstid;
{
}

static void
default_list_end (uiout, list_flag)
     struct ui_out *uiout;
     int list_flag;
{
}

static void
default_field_int (uiout, fldno, width, align, fldname, value)
     struct ui_out *uiout;
     int fldno;
     int width;
     enum ui_align align;
     char *fldname;
     int value;
{
}

static void
default_field_skip (uiout, fldno, width, align, fldname)
     struct ui_out *uiout;
     int fldno;
     int width;
     enum ui_align align;
     char *fldname;
{
}

static void
default_field_string (struct ui_out *uiout,
		      int fldno,
		      int width,
		      enum ui_align align,
		      char *fldname,
		      const char *string)
{
}

static void
default_field_fmt (uiout, fldno, width, align, fldname, format, args)
     struct ui_out *uiout;
     int fldno;
     int width;
     enum ui_align align;
     char *fldname;
     char *format;
     va_list args;
{
}

static void
default_spaces (uiout, numspaces)
     struct ui_out *uiout;
     int numspaces;
{
}

static void
default_text (uiout, string)
     struct ui_out *uiout;
     char *string;
{
}

static void
default_message (uiout, verbosity, format, args)
     struct ui_out *uiout;
     int verbosity;
     char *format;
     va_list args;
{
}

static void
default_wrap_hint (uiout, identstring)
     struct ui_out *uiout;
     char *identstring;
{
}

static void
default_flush (uiout)
     struct ui_out *uiout;
{
}

/* Interface to the implementation functions */

void
uo_table_begin (struct ui_out *uiout, int nbrofcols, char *tblid)
{
  if (!uiout->impl->table_begin)
    return;
  uiout->impl->table_begin (uiout, nbrofcols, tblid);
}

void
uo_table_body (struct ui_out *uiout)
{
  if (!uiout->impl->table_body)
    return;
  uiout->impl->table_body (uiout);
}

void
uo_table_end (struct ui_out *uiout)
{
  if (!uiout->impl->table_end)
    return;
  uiout->impl->table_end (uiout);
}

void
uo_table_header (struct ui_out *uiout, int width, enum ui_align align, char *colhdr)
{
  if (!uiout->impl->table_header)
    return;
  uiout->impl->table_header (uiout, width, align, colhdr);
}

void
uo_list_begin (struct ui_out *uiout, int list_flag, char *lstid)
{
  if (!uiout->impl->list_begin)
    return;
  uiout->impl->list_begin (uiout, list_flag, lstid);
}

void
uo_list_end (struct ui_out *uiout, int list_flag)
{
  if (!uiout->impl->list_end)
    return;
  uiout->impl->list_end (uiout, list_flag);
}

void
uo_field_int (struct ui_out *uiout, int fldno, int width, enum ui_align align, char *fldname, int value)
{
  if (!uiout->impl->field_int)
    return;
  uiout->impl->field_int (uiout, fldno, width, align, fldname, value);
}

void
uo_field_skip (struct ui_out *uiout, int fldno, int width, enum ui_align align, char *fldname)
{
  if (!uiout->impl->field_skip)
    return;
  uiout->impl->field_skip (uiout, fldno, width, align, fldname);
}

void
uo_field_string (struct ui_out *uiout, int fldno, int width,
		 enum ui_align align, char *fldname, const char *string)
{
  if (!uiout->impl->field_string)
    return;
  uiout->impl->field_string (uiout, fldno, width, align, fldname, string);
}

void
uo_field_fmt (struct ui_out *uiout, int fldno, int width, enum ui_align align, char *fldname, char *format, va_list args)
{
  if (!uiout->impl->field_fmt)
    return;
  uiout->impl->field_fmt (uiout, fldno, width, align, fldname, format, args);
}

void
uo_spaces (struct ui_out *uiout, int numspaces)
{
  if (!uiout->impl->spaces)
    return;
  uiout->impl->spaces (uiout, numspaces);
}

void
uo_text (struct ui_out *uiout, char *string)
{
  if (!uiout->impl->text)
    return;
  uiout->impl->text (uiout, string);
}

void
uo_message (struct ui_out *uiout, int verbosity, char *format, va_list args)
{
  if (!uiout->impl->message)
    return;
  uiout->impl->message (uiout, verbosity, format, args);
}

void
uo_wrap_hint (struct ui_out *uiout, char *identstring)
{
  if (!uiout->impl->wrap_hint)
    return;
  uiout->impl->wrap_hint (uiout, identstring);
}

void
uo_flush (struct ui_out *uiout)
{
  if (!uiout->impl->flush)
    return;
  uiout->impl->flush (uiout);
}

/* local functions */

/* list of column headers manipulation routines */

static void
clear_header_list (uiout)
     struct ui_out *uiout;
{
  while (uiout->headerfirst != NULL)
    {
      uiout->headercurr = uiout->headerfirst;
      uiout->headerfirst = uiout->headerfirst->next;
      if (uiout->headercurr->colhdr != NULL)
	free (uiout->headercurr->colhdr);
      free (uiout->headercurr);
    }
  uiout->headerlast = NULL;
  uiout->headercurr = NULL;
}

static void
append_header_to_list (struct ui_out *uiout,
		       int width,
		       int alignment,
		       char *colhdr)
{
  struct ui_out_hdr *temphdr;

  temphdr = XMALLOC (struct ui_out_hdr);
  temphdr->width = width;
  temphdr->alignment = alignment;
  /* we have to copy the column title as the original may be an automatic */
  if (colhdr != NULL)
    {
      temphdr->colhdr = xmalloc (strlen (colhdr) + 1);
      strcpy (temphdr->colhdr, colhdr);
    }
  temphdr->next = NULL;
  if (uiout->headerfirst == NULL)
    {
      temphdr->colno = 1;
      uiout->headerfirst = temphdr;
      uiout->headerlast = temphdr;
    }
  else
    {
      temphdr->colno = uiout->headerlast->colno + 1;
      uiout->headerlast->next = temphdr;
      uiout->headerlast = temphdr;
    }
  uiout->headercurr = uiout->headerlast;
}

/* returns 0 if there is no more headers */

static int
get_curr_header (struct ui_out *uiout,
		 int *colno,
		 int *width,
		 int *alignment,
		 char **colhdr)
{
  /* There may be no headers at all or we may have used all columns */
  if (uiout->headercurr == NULL)
    return 0;
  *colno = uiout->headercurr->colno;
  *width = uiout->headercurr->width;
  *alignment = uiout->headercurr->alignment;
  *colhdr = uiout->headercurr->colhdr;
  uiout->headercurr = uiout->headercurr->next;
  return 1;
}

/* makes sure the field_* calls were properly placed */

static void
verify_field_proper_position (struct ui_out *uiout)
{
  if (uiout->table_flag)
    {
      if (!uiout->body_flag)
	internal_error ("ui_out: table_body missing; table fields must be \
specified after table_body and inside a list.");
      if (!uiout->list_flag)
	internal_error ("ui_out: list_begin missing; table fields must be \
specified after table_body and inside a list.");
    }
}

/* determines what is the alignment policy */

static void
verify_field_alignment (struct ui_out *uiout,
			int fldno,
			int *width,
			int *align)
{
  int colno;
  char *text;

  if (uiout->table_flag
      && get_curr_header (uiout, &colno, width, align, &text))
    {
      if (fldno != colno)
	internal_error ("gdb/ui-out.c: ui-out internal error in handling headers.");
    }
  else
    {
      *width = 0;
      *align = ui_noalign;
    }
}

/* access to ui_out format private members */

void
ui_out_get_field_separator (uiout)
     struct ui_out *uiout;
{
}

/* Access to ui-out members data */

struct ui_out_data *
ui_out_data (struct ui_out *uiout)
{
  return uiout->data;
}

/* initalize private members at startup */

struct ui_out *
ui_out_new (struct ui_out_impl *impl,
	    struct ui_out_data *data,
	    int flags)
{
  struct ui_out *uiout = XMALLOC (struct ui_out);
  uiout->data = data;
  uiout->impl = impl;
  uiout->flags = flags;
  uiout->table_flag = 0;
  uiout->body_flag = 0;
  uiout->list_flag = 0;
  uiout->field_count[0] = 0;
  uiout->headerfirst = NULL;
  uiout->headerlast = NULL;
  uiout->headercurr = NULL;
  return uiout;
}

/* standard gdb initialization hook */

void
_initialize_ui_out ()
{
  /* nothing needs to be done */
}
