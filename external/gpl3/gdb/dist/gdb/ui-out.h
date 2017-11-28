/* Output generating routines for GDB.

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

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

#include "common/enum-flags.h"

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

extern struct cleanup *make_cleanup_ui_out_table_begin_end (struct ui_out *ui_out,
                                                            int nr_cols,
							    int nr_rows,
							    const char *tblid);
/* Compatibility wrappers.  */

extern struct cleanup *make_cleanup_ui_out_list_begin_end (struct ui_out *uiout,
							   const char *id);

extern struct cleanup *make_cleanup_ui_out_tuple_begin_end (struct ui_out *uiout,
							    const char *id);

class ui_out
{
 public:

  explicit ui_out (ui_out_flags flags = 0);
  virtual ~ui_out ();

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

  void field_int (const char *fldname, int value);
  void field_fmt_int (int width, ui_align align, const char *fldname,
		      int value);
  void field_core_addr (const char *fldname, struct gdbarch *gdbarch,
			CORE_ADDR address);
  void field_string (const char *fldname, const char *string);
  void field_stream (const char *fldname, string_file &stream);
  void field_skip (const char *fldname);
  void field_fmt (const char *fldname, const char *format, ...)
    ATTRIBUTE_PRINTF (3, 4);

  void spaces (int numspaces);
  void text (const char *string);
  void message (const char *format, ...) ATTRIBUTE_PRINTF (2, 3);
  void wrap_hint (const char *identstring);

  void flush ();

  /* Redirect the output of a ui_out object temporarily.  */
  void redirect (ui_file *outstream);

  ui_out_flags test_flags (ui_out_flags mask);

  /* HACK: Code in GDB is currently checking to see the type of ui_out
     builder when determining which output to produce.  This function is
     a hack to encapsulate that test.  Once GDB manages to separate the
     CLI/MI from the core of GDB the problem should just go away ....  */

  bool is_mi_like_p ();

  bool query_table_field (int colno, int *width, int *alignment,
			  const char **col_name);

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
  virtual void do_field_int (int fldno, int width, ui_align align,
			     const char *fldname, int value) = 0;
  virtual void do_field_skip (int fldno, int width, ui_align align,
			      const char *fldname) = 0;
  virtual void do_field_string (int fldno, int width, ui_align align,
				const char *fldname, const char *string) = 0;
  virtual void do_field_fmt (int fldno, int width, ui_align align,
			     const char *fldname, const char *format,
			     va_list args)
    ATTRIBUTE_PRINTF (6,0) = 0;
  virtual void do_spaces (int numspaces) = 0;
  virtual void do_text (const char *string) = 0;
  virtual void do_message (const char *format, va_list args)
    ATTRIBUTE_PRINTF (2,0) = 0;
  virtual void do_wrap_hint (const char *identstring) = 0;
  virtual void do_flush () = 0;
  virtual void do_redirect (struct ui_file *outstream) = 0;

  /* Set as not MI-like by default.  It is overridden in subclasses if
     necessary.  */

  virtual bool do_is_mi_like_p ()
  { return false; }

 private:

  ui_out_flags m_flags;

  /* Vector to store and track the ui-out levels.  */
  std::vector<std::unique_ptr<ui_out_level>> m_levels;

  /* A table, if any.  At present only a single table is supported.  */
  std::unique_ptr<ui_out_table> m_table_up;

  void verify_field (int *fldno, int *width, ui_align *align);

  int level () const;
  ui_out_level *current_level () const;
};

/* This is similar to make_cleanup_ui_out_tuple_begin_end and
   make_cleanup_ui_out_list_begin_end, but written as an RAII template
   class.  It takes the ui_out_type as a template parameter.  Normally
   this is used via the typedefs ui_out_emit_tuple and
   ui_out_emit_list.  */
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

  ui_out_emit_type (const ui_out_emit_type<Type> &) = delete;
  ui_out_emit_type<Type> &operator= (const ui_out_emit_type<Type> &)
      = delete;

private:

  struct ui_out *m_uiout;
};

typedef ui_out_emit_type<ui_out_type_tuple> ui_out_emit_tuple;
typedef ui_out_emit_type<ui_out_type_list> ui_out_emit_list;

#endif /* UI_OUT_H */
