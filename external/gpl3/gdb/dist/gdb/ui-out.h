/* Output generating routines for GDB.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions.
   Written by Fernando Nasser for Cygnus.

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

#ifndef UI_OUT_H
#define UI_OUT_H 1

#include <vector>

#include "gdbsupport/enum-flags.h"
#include "ui-style.h"

class ui_out_level;
class ui_out_table;
struct ui_file;

/* the current ui_out */

/* FIXME: This should not be a global but something passed down from main.c
   or top.c.  */
extern struct ui_out **current_ui_current_uiout_ptr (void);
#define current_uiout (*current_ui_current_uiout_ptr ())

/* alignment enum */
enum ui_align
  {
    ui_left = -1,
    ui_center,
    ui_right,
    ui_noalign
  };

/* flags enum */
enum ui_out_flag
{
  ui_source_list = (1 << 0),
  fix_multi_location_breakpoint_output = (1 << 1),
  /* This indicates that %pF should be disallowed in a printf format
     string.  */
  disallow_ui_out_field = (1 << 2),
  fix_breakpoint_script_output = (1 << 3),
};

DEF_ENUM_FLAGS_TYPE (ui_out_flag, ui_out_flags);

/* Prototypes for ui-out API.  */

/* A result is a recursive data structure consisting of lists and
   tuples.  */

enum ui_out_type
  {
    ui_out_type_tuple,
    ui_out_type_list
  };

/* The possible kinds of fields.  */
enum class field_kind
  {
    /* "FIELD_STRING" needs a funny name to avoid clashes with tokens
       named "STRING".  See PR build/25250.  FIELD_SIGNED is given a
       similar name for consistency.  */
    FIELD_SIGNED,
    FIELD_STRING,
  };

/* The base type of all fields that can be emitted using %pF.  */

struct base_field_s
{
  const char *name;
  field_kind kind;
};

/* A signed integer field, to be passed to %pF in format strings.  */

struct signed_field_s : base_field_s
{
  LONGEST val;
};

/* Construct a temporary signed_field_s on the caller's stack and
   return a pointer to the constructed object.  We use this because
   it's not possible to pass a reference via va_args.  */

static inline signed_field_s *
signed_field (const char *name, LONGEST val,
	      signed_field_s &&tmp = {})
{
  tmp.name = name;
  tmp.kind = field_kind::FIELD_SIGNED;
  tmp.val = val;
  return &tmp;
}

/* A string field, to be passed to %pF in format strings.  */

struct string_field_s : base_field_s
{
  const char *str;
};

/* Construct a temporary string_field_s on the caller's stack and
   return a pointer to the constructed object.  We use this because
   it's not possible to pass a reference via va_args.  */

static inline string_field_s *
string_field (const char *name, const char *str,
	      string_field_s &&tmp = {})
{
  tmp.name = name;
  tmp.kind = field_kind::FIELD_STRING;
  tmp.str = str;
  return &tmp;
}

/* A styled string.  */

struct styled_string_s
{
  /* The style.  */
  ui_file_style style;

  /* The string.  */
  const char *str;
};

/* Construct a temporary styled_string_s on the caller's stack and
   return a pointer to the constructed object.  We use this because
   it's not possible to pass a reference via va_args.  */

static inline styled_string_s *
styled_string (const ui_file_style &style, const char *str,
	       styled_string_s &&tmp = {})
{
  tmp.style = style;
  tmp.str = str;
  return &tmp;
}

class ui_out
{
 public:

  explicit ui_out (ui_out_flags flags = 0);
  virtual ~ui_out ();

  DISABLE_COPY_AND_ASSIGN (ui_out);

  void push_level (ui_out_type type);
  void pop_level (ui_out_type type);

  /* A table can be considered a special tuple/list combination with the
     implied structure: ``table = { hdr = { header, ... } , body = [ {
     field, ... }, ... ] }''.  If NR_ROWS is negative then there is at
     least one row.  */

  void table_begin (int nr_cols, int nr_rows, const std::string &tblid);
  void table_header (int width, ui_align align, const std::string &col_name,
		     const std::string &col_hdr);
  void table_body ();
  void table_end ();

  void begin (ui_out_type type, const char *id);
  void end (ui_out_type type);

  void field_signed (const char *fldname, LONGEST value);
  void field_fmt_signed (int width, ui_align align, const char *fldname,
			 LONGEST value);
  /* Like field_signed, but print an unsigned value.  */
  void field_unsigned (const char *fldname, ULONGEST value);
  void field_core_addr (const char *fldname, struct gdbarch *gdbarch,
			CORE_ADDR address);
  void field_string (const char *fldname, const char *string,
		     const ui_file_style &style = ui_file_style ());
  void field_string (const char *fldname, const std::string &string,
		     const ui_file_style &style = ui_file_style ())
  {
    field_string (fldname, string.c_str (), style);
  }
  void field_stream (const char *fldname, string_file &stream,
		     const ui_file_style &style = ui_file_style ());
  void field_skip (const char *fldname);
  void field_fmt (const char *fldname, const char *format, ...)
    ATTRIBUTE_PRINTF (3, 4);
  void field_fmt (const char *fldname, const ui_file_style &style,
		  const char *format, ...)
    ATTRIBUTE_PRINTF (4, 5);

  void spaces (int numspaces) { do_spaces (numspaces); }
  void text (const char *string) { do_text (string); }
  void text (const std::string &string) { text (string.c_str ()); }

  /* Output a printf-style formatted string.  In addition to the usual
     printf format specs, this supports a few GDB-specific
     formatters:

     - '%pF' - output a field.

       The argument is a field, wrapped in any of the base_field_s
       subclasses.  signed_field for integer fields, string_field for
       string fields.  This is preferred over separate
       uiout->field_signed(), uiout_>field_string() etc. calls when
       the formatted message is translatable.  E.g.:

	 uiout->message (_("\nWatchpoint %pF deleted because the program has "
			 "left the block in\n"
			 "which its expression is valid.\n"),
			 signed_field ("wpnum", b->number));

     - '%p[' - output the following text in a specified style.
       '%p]' - output the following text in the default style.

       The argument to '%p[' is a ui_file_style pointer.  The argument
       to '%p]' must be nullptr.

       This is useful when you want to output some portion of a string
       literal in some style.  E.g.:

	 uiout->message (_(" %p[<repeats %u times>%p]"),
			 metadata_style.style ().ptr (),
			 reps, repeats, nullptr);

     - '%ps' - output a styled string.

       The argument is the result of a call to styled_string.  This is
       useful when you want to output some runtime-generated string in
       some style.  E.g.:

	 uiout->message (_("this is a target address %ps.\n"),
			 styled_string (address_style.style (),
					paddress (gdbarch, pc)));

     Note that these all "abuse" the %p printf format spec, in order
     to be compatible with GCC's printf format checking.  This is OK
     because code in GDB that wants to print a host address should use
     host_address_to_string instead of %p.  */
  void message (const char *format, ...) ATTRIBUTE_PRINTF (2, 3);
  void vmessage (const ui_file_style &in_style,
		 const char *format, va_list args) ATTRIBUTE_PRINTF (3, 0);

  void wrap_hint (int indent) { do_wrap_hint (indent); }

  void flush () { do_flush (); }

  /* Redirect the output of a ui_out object temporarily.  */
  void redirect (ui_file *outstream) { do_redirect (outstream); }

  ui_out_flags test_flags (ui_out_flags mask)
  { return m_flags & mask; }

  /* HACK: Code in GDB is currently checking to see the type of ui_out
     builder when determining which output to produce.  This function is
     a hack to encapsulate that test.  Once GDB manages to separate the
     CLI/MI from the core of GDB the problem should just go away ....  */

  bool is_mi_like_p () const { return do_is_mi_like_p (); }

  bool query_table_field (int colno, int *width, int *alignment,
			  const char **col_name);

  /* Return true if this stream is prepared to handle style
     escapes.  */
  virtual bool can_emit_style_escape () const = 0;

  /* Return the ui_file currently used for output.  */
  virtual ui_file *current_stream () const = 0;

  /* An object that starts and finishes displaying progress updates.  */
  class progress_update
  {
  public:
    /* Represents the printing state of a progress update.  */
    enum state
    {
      /* Printing will start with the next update.  */
      START,
      /* Printing has already started.  */
      WORKING,
      /* Progress bar printing has already started.  */
      BAR
    };

    /* SHOULD_PRINT indicates whether something should be printed for a tty.  */
    progress_update ()
    {
      m_uiout = current_uiout;
      m_uiout->do_progress_start ();
    }

    ~progress_update ()
    {
      m_uiout->do_progress_end ();
    }

    progress_update (const progress_update &) = delete;
    progress_update &operator= (const progress_update &) = delete;

    /* Emit some progress for this progress meter.  Includes current
       amount of progress made and total amount in the display.  */
    void update_progress (const std::string& msg, const char *unit,
			  double cur, double total)
    {
      m_uiout->do_progress_notify (msg, unit, cur, total);
    }

    /* Emit some progress for this progress meter.  */
    void update_progress (const std::string& msg)
    {
      m_uiout->do_progress_notify (msg, "", -1, -1);
    }

  private:

    struct ui_out *m_uiout;
  };

protected:

  virtual void do_table_begin (int nbrofcols, int nr_rows, const char *tblid)
    = 0;
  virtual void do_table_body () = 0;
  virtual void do_table_end () = 0;
  virtual void do_table_header (int width, ui_align align,
				const std::string &col_name,
				const std::string &col_hdr) = 0;

  virtual void do_begin (ui_out_type type, const char *id) = 0;
  virtual void do_end (ui_out_type type) = 0;
  virtual void do_field_signed (int fldno, int width, ui_align align,
				const char *fldname, LONGEST value) = 0;
  virtual void do_field_unsigned (int fldno, int width, ui_align align,
				  const char *fldname, ULONGEST value) = 0;
  virtual void do_field_skip (int fldno, int width, ui_align align,
			      const char *fldname) = 0;
  virtual void do_field_string (int fldno, int width, ui_align align,
				const char *fldname, const char *string,
				const ui_file_style &style) = 0;
  virtual void do_field_fmt (int fldno, int width, ui_align align,
			     const char *fldname, const ui_file_style &style,
			     const char *format, va_list args)
    ATTRIBUTE_PRINTF (7, 0) = 0;
  virtual void do_spaces (int numspaces) = 0;
  virtual void do_text (const char *string) = 0;
  virtual void do_message (const ui_file_style &style,
			   const char *format, va_list args)
    ATTRIBUTE_PRINTF (3,0) = 0;
  virtual void do_wrap_hint (int indent) = 0;
  virtual void do_flush () = 0;
  virtual void do_redirect (struct ui_file *outstream) = 0;

  virtual void do_progress_start () = 0;
  virtual void do_progress_notify (const std::string &, const char *,
				   double, double) = 0;
  virtual void do_progress_end () = 0;

  /* Set as not MI-like by default.  It is overridden in subclasses if
     necessary.  */

  virtual bool do_is_mi_like_p () const
  { return false; }

 private:
  void call_do_message (const ui_file_style &style, const char *format,
			...);

  ui_out_flags m_flags;

  /* Vector to store and track the ui-out levels.  */
  std::vector<std::unique_ptr<ui_out_level>> m_levels;

  /* A table, if any.  At present only a single table is supported.  */
  std::unique_ptr<ui_out_table> m_table_up;

  void verify_field (int *fldno, int *width, ui_align *align);

  int level () const;
  ui_out_level *current_level () const;
};

/* Start a new tuple or list on construction, and end it on
   destruction.  Normally this is used via the typedefs
   ui_out_emit_tuple and ui_out_emit_list.  */
template<ui_out_type Type>
class ui_out_emit_type
{
public:

  ui_out_emit_type (struct ui_out *uiout, const char *id)
    : m_uiout (uiout)
  {
    uiout->begin (Type, id);
  }

  ~ui_out_emit_type ()
  {
    m_uiout->end (Type);
  }

  DISABLE_COPY_AND_ASSIGN (ui_out_emit_type);

private:

  struct ui_out *m_uiout;
};

typedef ui_out_emit_type<ui_out_type_tuple> ui_out_emit_tuple;
typedef ui_out_emit_type<ui_out_type_list> ui_out_emit_list;

/* Start a new table on construction, and end the table on
   destruction.  */
class ui_out_emit_table
{
public:

  ui_out_emit_table (struct ui_out *uiout, int nr_cols, int nr_rows,
		     const char *tblid)
    : m_uiout (uiout)
  {
    m_uiout->table_begin (nr_cols, nr_rows, tblid);
  }

  ~ui_out_emit_table ()
  {
    m_uiout->table_end ();
  }

  ui_out_emit_table (const ui_out_emit_table &) = delete;
  ui_out_emit_table &operator= (const ui_out_emit_table &) = delete;

private:

  struct ui_out *m_uiout;
};

/* On construction, redirect a uiout to a given stream.  On
   destruction, pop the last redirection by calling the uiout's
   redirect method with a NULL parameter.  */
class ui_out_redirect_pop
{
public:

  ui_out_redirect_pop (ui_out *uiout, ui_file *stream)
    : m_uiout (uiout)
  {
    m_uiout->redirect (stream);
  }

  ~ui_out_redirect_pop ()
  {
    m_uiout->redirect (NULL);
  }

  ui_out_redirect_pop (const ui_out_redirect_pop &) = delete;
  ui_out_redirect_pop &operator= (const ui_out_redirect_pop &) = delete;

private:
  struct ui_out *m_uiout;
};

struct buffered_streams;

/* Organizes writes to a collection of buffered output streams
   so that when flushed, output is written to all streams in
   chronological order.  */

struct buffer_group
{
  buffer_group (ui_out *uiout);

  /* Flush all buffered writes to the underlying output streams.  */
  void flush () const;

  /* Record contents of BUF and associate it with STREAM.  */
  void write (const char *buf, long length_buf, ui_file *stream);

  /* Record a wrap_here and associate it with STREAM.  */
  void wrap_here (int indent, ui_file *stream);

  /* Record a call to flush and associate it with STREAM.  */
  void flush_here (ui_file *stream);

private:

  struct output_unit
  {
    output_unit (std::string msg, int wrap_hint = -1, bool flush = false)
      : m_msg (msg), m_wrap_hint (wrap_hint), m_flush (flush)
    {}

    /* Write contents of this output_unit to the underlying stream.  */
    void flush () const;

    /* Underlying stream for which this output unit will be written to.  */
    ui_file *m_stream;

    /* String to be written to underlying buffer.  */
    std::string m_msg;

    /* Argument to wrap_here.  -1 indicates no wrap.  Used to call wrap_here
       during buffer flush.  */
    int m_wrap_hint;

    /* Indicate that the underlying output stream's flush should be called.  */
    bool m_flush;
  };

  /* Output_units to be written to buffered output streams.  */
  std::vector<output_unit> m_buffered_output;

  /* Buffered output streams.  */
  std::unique_ptr<buffered_streams> m_buffered_streams;
};

/* If FILE is a buffering_file, return it's underlying stream.  */

extern ui_file *get_unbuffered (ui_file *file);

/* Buffer output to gdb_stdout and gdb_stderr for the duration of FUNC.  */

template<typename F, typename... Arg>
void
do_with_buffered_output (F func, ui_out *uiout, Arg... args)
{
  buffer_group g (uiout);

  try
    {
      func (uiout, std::forward<Arg> (args)...);
    }
  catch (gdb_exception &ex)
    {
      /* Ideally flush would be called in the destructor of buffer_group,
	 however flushing might cause an exception to be thrown.  Catch it
	 and ensure the first exception propagates.  */
      try
	{
	  g.flush ();
	}
      catch (const gdb_exception &)
	{
	}

      throw_exception (std::move (ex));
    }

  /* Try was successful.  Let any further exceptions propagate.  */
  g.flush ();
}

/* Accumulate writes to an underlying ui_file.  Output to the
   underlying file is deferred until required.  */

struct buffering_file : public ui_file
{
  buffering_file (buffer_group *group, ui_file *stream)
    : m_group (group),
      m_stream (stream)
  { /* Nothing.  */ }

  /* Return the underlying output stream.  */
  ui_file *stream () const
  {
    return m_stream;
  }

  /* Record the contents of BUF.  */
  void write (const char *buf, long length_buf) override
  {
    m_group->write (buf, length_buf, m_stream);
  }

  /* Record a wrap_here call with argument INDENT.  */
  void wrap_here (int indent) override
  {
    m_group->wrap_here (indent, m_stream);
  }

  /* Return true if the underlying stream is a tty.  */
  bool isatty () override
  {
    return m_stream->isatty ();
  }

  /* Return true if ANSI escapes can be used on the underlying stream.  */
  bool can_emit_style_escape () override
  {
    return m_stream->can_emit_style_escape ();
  }

  /* Flush the underlying output stream.  */
  void flush () override
  {
    return m_group->flush_here (m_stream);
  }

private:

  /* Coordinates buffering across multiple buffering_files.  */
  buffer_group *m_group;

  /* The underlying output stream.  */
  ui_file *m_stream;
};

/* Attaches and detaches buffers for each of the gdb_std* streams.  */

struct buffered_streams
{
  buffered_streams (buffer_group *group, ui_out *uiout);

  ~buffered_streams ()
  {
    this->remove_buffers ();
  }

  /* Remove buffering_files from all underlying streams.  */
  void remove_buffers ();

private:

  /* True if buffers are still attached to each underlying output stream.  */
  bool m_buffers_in_place;

  /* Buffers for each gdb_std* output stream.  */
  buffering_file m_buffered_stdout;
  buffering_file m_buffered_stderr;
  buffering_file m_buffered_stdlog;
  buffering_file m_buffered_stdtarg;

  /* Buffer for current_uiout's output stream.  */
  std::optional<buffering_file> m_buffered_current_uiout;

  /* Additional ui_out being buffered.  */
  ui_out *m_uiout;

  /* Buffer for m_uiout's output stream.  */
  std::optional<buffering_file> m_buffered_uiout;
};

#endif /* UI_OUT_H */
