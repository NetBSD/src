/*	$NetBSD: ps.cpp,v 1.2 2016/01/13 19:01:58 christos Exp $	*/

// -*- C++ -*-
/* Copyright (C) 1989, 1990, 1991, 1992, 2000, 2001, 2002, 2003, 2004
   Free Software Foundation, Inc.
     Written by James Clark (jjc@jclark.com)

This file is part of groff.

groff is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

groff is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with groff; see the file COPYING.  If not, write to the Free Software
Foundation, 51 Franklin St - Fifth Floor, Boston, MA 02110-1301, USA. */

/*
 * PostScript documentation:
 *   http://www.adobe.com/products/postscript/pdfs/PLRM.pdf
 *   http://partners.adobe.com/asn/developer/pdfs/tn/5001.DSC_Spec.pdf
 */

#include "driver.h"
#include "stringclass.h"
#include "cset.h"
#include "nonposix.h"
#include "paper.h"

#include "ps.h"
#include <time.h>

#ifdef NEED_DECLARATION_PUTENV
extern "C" {
  int putenv(const char *);
}
#endif /* NEED_DECLARATION_PUTENV */

extern "C" const char *Version_string;

// search path defaults to the current directory
search_path include_search_path(0, 0, 0, 1);

static int landscape_flag = 0;
static int manual_feed_flag = 0;
static int ncopies = 1;
static int linewidth = -1;
// Non-zero means generate PostScript code that guesses the paper
// length using the imageable area.
static int guess_flag = 0;
static double user_paper_length = 0;
static double user_paper_width = 0;

// Non-zero if -b was specified on the command line.
static int bflag = 0;
unsigned broken_flags = 0;

// Non-zero means we need the CMYK extension for PostScript Level 1
static int cmyk_flag = 0;

#define DEFAULT_LINEWIDTH 40	/* in ems/1000 */
#define MAX_LINE_LENGTH 72
#define FILL_MAX 1000

const char *const dict_name = "grops";
const char *const defs_dict_name = "DEFS";
const int DEFS_DICT_SPARE = 50;

double degrees(double r)
{
  return r*180.0/PI;
}

double radians(double d)
{
  return d*PI/180.0;
}

// This is used for testing whether a character should be output in the
// PostScript file using \nnn, so we really want the character to be
// less than 0200.

inline int is_ascii(char c)
{
  return (unsigned char)c < 0200;
}

ps_output::ps_output(FILE *f, int n)
: fp(f), col(0), max_line_length(n), need_space(0), fixed_point(0)
{
}

ps_output &ps_output::set_file(FILE *f)
{
  fp = f;
  col = 0;
  return *this;
}

ps_output &ps_output::copy_file(FILE *infp)
{
  int c;
  while ((c = getc(infp)) != EOF)
    putc(c, fp);
  return *this;
}

ps_output &ps_output::end_line()
{
  if (col != 0) {
    putc('\n', fp);
    col = 0;
    need_space = 0;
  }
  return *this;
}

ps_output &ps_output::special(const char *s)
{
  if (s == 0 || *s == '\0')
    return *this;
  if (col != 0) {
    putc('\n', fp);
    col = 0;
  }
  fputs(s, fp);
  if (strchr(s, '\0')[-1] != '\n')
    putc('\n', fp);
  need_space = 0;
  return *this;
}

ps_output &ps_output::simple_comment(const char *s)
{
  if (col != 0)
    putc('\n', fp);
  putc('%', fp);
  putc('%', fp);
  fputs(s, fp);
  putc('\n', fp);
  col = 0;
  need_space = 0;
  return *this;
}

ps_output &ps_output::begin_comment(const char *s)
{
  if (col != 0)
    putc('\n', fp);
  putc('%', fp);
  putc('%', fp);
  fputs(s, fp);
  col = 2 + strlen(s);
  return *this;
}

ps_output &ps_output::end_comment()
{
  if (col != 0) {
    putc('\n', fp);
    col = 0;
  }
  need_space = 0;
  return *this;
}

ps_output &ps_output::comment_arg(const char *s)
{
  int len = strlen(s);
  if (col + len + 1 > max_line_length) {
    putc('\n', fp);
    fputs("%%+", fp);
    col = 3;
  }
  putc(' ',  fp);
  fputs(s, fp);
  col += len + 1;
  return *this;
}

ps_output &ps_output::set_fixed_point(int n)
{
  assert(n >= 0 && n <= 10);
  fixed_point = n;
  return *this;
}

ps_output &ps_output::put_delimiter(char c)
{
  if (col + 1 > max_line_length) {
    putc('\n', fp);
    col = 0;
  }
  putc(c, fp);
  col++;
  need_space = 0;
  return *this;
}

ps_output &ps_output::put_string(const char *s, int n)
{
  int len = 0;
  int i;
  for (i = 0; i < n; i++) {
    char c = s[i];
    if (is_ascii(c) && csprint(c)) {
      if (c == '(' || c == ')' || c == '\\')
	len += 2;
      else
	len += 1;
    }
    else
      len += 4;
  }
  if (len > n*2) {
    if (col + n*2 + 2 > max_line_length && n*2 + 2 <= max_line_length) {
      putc('\n', fp);
      col = 0;
    }
    if (col + 1 > max_line_length) {
      putc('\n', fp);
      col = 0;
    }
    putc('<', fp);
    col++;
    for (i = 0; i < n; i++) {
      if (col + 2 > max_line_length) {
	putc('\n', fp);
	col = 0;
      }
      fprintf(fp, "%02x", s[i] & 0377);
      col += 2;
    }
    putc('>', fp);
    col++;
  }
  else {
    if (col + len + 2 > max_line_length && len + 2 <= max_line_length) {
      putc('\n', fp);
      col = 0;
    }
    if (col + 2 > max_line_length) {
      putc('\n', fp);
      col = 0;
    }
    putc('(', fp);
    col++;
    for (i = 0; i < n; i++) {
      char c = s[i];
      if (is_ascii(c) && csprint(c)) {
	if (c == '(' || c == ')' || c == '\\')
	  len = 2;
	else
	  len = 1;
      }
      else
	len = 4;
      if (col + len + 1 > max_line_length) {
	putc('\\', fp);
	putc('\n', fp);
	col = 0;
      }
      switch (len) {
      case 1:
	putc(c, fp);
	break;
      case 2:
	putc('\\', fp);
	putc(c, fp);
	break;
      case 4:
	fprintf(fp, "\\%03o", c & 0377);
	break;
      default:
	assert(0);
      }
      col += len;
    }
    putc(')', fp);
    col++;
  }
  need_space = 0;
  return *this;
}

ps_output &ps_output::put_number(int n)
{
  char buf[1 + INT_DIGITS + 1];
  sprintf(buf, "%d", n);
  int len = strlen(buf);
  if (col > 0 && col + len + need_space > max_line_length) {
    putc('\n', fp);
    col = 0;
    need_space = 0;
  }
  if (need_space) {
    putc(' ', fp);
    col++;
  }
  fputs(buf, fp);
  col += len;
  need_space = 1;
  return *this;
}

ps_output &ps_output::put_fix_number(int i)
{
  const char *p = if_to_a(i, fixed_point);
  int len = strlen(p);
  if (col > 0 && col + len + need_space > max_line_length) {
    putc('\n', fp);
    col = 0;
    need_space = 0;
  }
  if (need_space) {
    putc(' ', fp);
    col++;
  }
  fputs(p, fp);
  col += len;
  need_space = 1;
  return *this;
}

ps_output &ps_output::put_float(double d)
{
  char buf[128];
  sprintf(buf, "%.4f", d);
  int last = strlen(buf) - 1;
  while (buf[last] == '0')
    last--;
  if (buf[last] == '.')
    last--;
  buf[++last] = '\0';
  if (col > 0 && col + last + need_space > max_line_length) {
    putc('\n', fp);
    col = 0;
    need_space = 0;
  }
  if (need_space) {
    putc(' ', fp);
    col++;
  }
  fputs(buf, fp);
  col += last;
  need_space = 1;
  return *this;
}

ps_output &ps_output::put_symbol(const char *s)
{
  int len = strlen(s);
  if (col > 0 && col + len + need_space > max_line_length) {
    putc('\n', fp);
    col = 0;
    need_space = 0;
  }
  if (need_space) {
    putc(' ', fp);
    col++;
  }
  fputs(s, fp);
  col += len;
  need_space = 1;
  return *this;
}

ps_output &ps_output::put_color(unsigned int c)
{
  char buf[128];
  sprintf(buf, "%.3g", double(c) / color::MAX_COLOR_VAL);
  int len = strlen(buf);
  if (col > 0 && col + len + need_space > max_line_length) {
    putc('\n', fp);
    col = 0;
    need_space = 0;
  }
  if (need_space) {
    putc(' ', fp);
    col++;
  }
  fputs(buf, fp);
  col += len;
  need_space = 1;
  return *this;
}

ps_output &ps_output::put_literal_symbol(const char *s)
{
  int len = strlen(s);
  if (col > 0 && col + len + 1 > max_line_length) {
    putc('\n', fp);
    col = 0;
  }
  putc('/', fp);
  fputs(s, fp);
  col += len + 1;
  need_space = 1;
  return *this;
}

class ps_font : public font {
  ps_font(const char *);
public:
  int encoding_index;
  char *encoding;
  char *reencoded_name;
  ~ps_font();
  void handle_unknown_font_command(const char *command, const char *arg,
				   const char *filename, int lineno);
  static ps_font *load_ps_font(const char *);
};

ps_font *ps_font::load_ps_font(const char *s)
{
  ps_font *f = new ps_font(s);
  if (!f->load()) {
    delete f;
    return 0;
  }
  return f;
}

ps_font::ps_font(const char *nm)
: font(nm), encoding_index(-1), encoding(0), reencoded_name(0)
{
}

ps_font::~ps_font()
{
  a_delete encoding;
  a_delete reencoded_name;
}

void ps_font::handle_unknown_font_command(const char *command, const char *arg,
					  const char *filename, int lineno)
{
  if (strcmp(command, "encoding") == 0) {
    if (arg == 0)
      error_with_file_and_line(filename, lineno,
			       "`encoding' command requires an argument");
    else
      encoding = strsave(arg);
  }
}

static void handle_unknown_desc_command(const char *command, const char *arg,
					const char *filename, int lineno)
{
  if (strcmp(command, "broken") == 0) {
    if (arg == 0)
      error_with_file_and_line(filename, lineno,
			       "`broken' command requires an argument");
    else if (!bflag)
      broken_flags = atoi(arg);
  }
}

struct subencoding {
  font *p;
  unsigned int num;
  int idx;
  char *subfont;
  const char *glyphs[256];
  subencoding *next;

  subencoding(font *, unsigned int, int, subencoding *);
  ~subencoding();
};

subencoding::subencoding(font *f, unsigned int n, int ix, subencoding *s)
: p(f), num(n), idx(ix), subfont(0), next(s)
{
  for (int i = 0; i < 256; i++)
    glyphs[i] = 0;
}

subencoding::~subencoding()
{
  a_delete subfont;
}

struct style {
  font *f;
  subencoding *sub;
  int point_size;
  int height;
  int slant;
  style();
  style(font *, subencoding *, int, int, int);
  int operator==(const style &) const;
  int operator!=(const style &) const;
};

style::style() : f(0)
{
}

style::style(font *p, subencoding *s, int sz, int h, int sl)
: f(p), sub(s), point_size(sz), height(h), slant(sl)
{
}

int style::operator==(const style &s) const
{
  return (f == s.f
	  && sub == s.sub
	  && point_size == s.point_size
	  && height == s.height
	  && slant == s.slant);
}

int style::operator!=(const style &s) const
{
  return !(*this == s);
}

class ps_printer : public printer {
  FILE *tempfp;
  ps_output out;
  int res;
  int space_char_index;
  int pages_output;
  int paper_length;
  int equalise_spaces;
  enum { SBUF_SIZE = 256 };
  char sbuf[SBUF_SIZE];
  int sbuf_len;
  int sbuf_start_hpos;
  int sbuf_vpos;
  int sbuf_end_hpos;
  int sbuf_space_width;
  int sbuf_space_count;
  int sbuf_space_diff_count;
  int sbuf_space_code;
  int sbuf_kern;
  style sbuf_style;
  color sbuf_color;		// the current PS color
  style output_style;
  subencoding *subencodings;
  int output_hpos;
  int output_vpos;
  int output_draw_point_size;
  int line_thickness;
  int output_line_thickness;
  unsigned char output_space_code;
  enum { MAX_DEFINED_STYLES = 50 };
  style defined_styles[MAX_DEFINED_STYLES];
  int ndefined_styles;
  int next_encoding_index;
  int next_subencoding_index;
  string defs;
  int ndefs;
  resource_manager rm;
  int invis_count;

  void flush_sbuf();
  void set_style(const style &);
  void set_space_code(unsigned char c);
  int set_encoding_index(ps_font *);
  subencoding *set_subencoding(font *, int, unsigned char *);
  char *get_subfont(subencoding *, const char *);
  void do_exec(char *, const environment *);
  void do_import(char *, const environment *);
  void do_def(char *, const environment *);
  void do_mdef(char *, const environment *);
  void do_file(char *, const environment *);
  void do_invis(char *, const environment *);
  void do_endinvis(char *, const environment *);
  void set_line_thickness_and_color(const environment *);
  void fill_path(const environment *);
  void encode_fonts();
  void encode_subfont(subencoding *);
  void define_encoding(const char *, int);
  void reencode_font(ps_font *);
  void set_color(color *c, int fill = 0);

  const char *media_name();
  int media_width();
  int media_height();
  void media_set();

public:
  ps_printer(double);
  ~ps_printer();
  void set_char(int i, font *f, const environment *env, int w,
		const char *name);
  void draw(int code, int *p, int np, const environment *env);
  void begin_page(int);
  void end_page(int);
  void special(char *arg, const environment *env, char type);
  font *make_font(const char *);
  void end_of_line();
};

// `pl' is in inches
ps_printer::ps_printer(double pl)
: out(0, MAX_LINE_LENGTH),
  pages_output(0),
  sbuf_len(0),
  subencodings(0),
  output_hpos(-1),
  output_vpos(-1),
  line_thickness(-1),
  ndefined_styles(0),
  next_encoding_index(0),
  next_subencoding_index(0),
  ndefs(0),
  invis_count(0)
{
  tempfp = xtmpfile();
  out.set_file(tempfp);
  if (linewidth < 0)
    linewidth = DEFAULT_LINEWIDTH;
  if (font::hor != 1)
    fatal("horizontal resolution must be 1");
  if (font::vert != 1)
    fatal("vertical resolution must be 1");
  if (font::res % (font::sizescale*72) != 0)
    fatal("res must be a multiple of 72*sizescale");
  int r = font::res;
  int point = 0;
  while (r % 10 == 0) {
    r /= 10;
    point++;
  }
  res = r;
  out.set_fixed_point(point);
  space_char_index = font::name_to_index("space");
  if (pl == 0)
    paper_length = font::paperlength;
  else
    paper_length = int(pl * font::res + 0.5);
  if (paper_length == 0)
    paper_length = 11 * font::res;
  equalise_spaces = font::res >= 72000;
}

int ps_printer::set_encoding_index(ps_font *f)
{
  if (f->encoding_index >= 0)
    return f->encoding_index;
  for (font_pointer_list *p = font_list; p; p = p->next)
    if (p->p != f) {
      char *encoding = ((ps_font *)p->p)->encoding;
      int encoding_index = ((ps_font *)p->p)->encoding_index;
      if (encoding != 0 && encoding_index >= 0 
	  && strcmp(f->encoding, encoding) == 0) {
	return f->encoding_index = encoding_index;
      }
    }
  return f->encoding_index = next_encoding_index++;
}

subencoding *ps_printer::set_subencoding(font *f, int i, unsigned char *codep)
{
  unsigned int idx = f->get_code(i);
  *codep = idx % 256;
  unsigned int num = idx >> 8;
  if (num == 0)
    return 0;
  subencoding *p = 0;
  for (p = subencodings; p; p = p->next)
    if (p->p == f && p->num == num)
      break;
  if (p == 0)
    p = subencodings = new subencoding(f, num, next_subencoding_index++,
				       subencodings);
  p->glyphs[*codep] = f->get_special_device_encoding(i);
  return p;
}

char *ps_printer::get_subfont(subencoding *sub, const char *stem)
{
  assert(sub != 0);
  if (!sub->subfont) {
    char *tem = new char[strlen(stem) + 2 + INT_DIGITS + 1];
    sprintf(tem, "%s@@%d", stem, next_subencoding_index);
    sub->subfont = tem;
  }
  return sub->subfont;
}

void ps_printer::set_char(int i, font *f, const environment *env, int w,
			  const char *)
{
  if (i == space_char_index || invis_count > 0)
    return;
  unsigned char code;
  subencoding *sub = set_subencoding(f, i, &code);
  style sty(f, sub, env->size, env->height, env->slant);
  if (sty.slant != 0) {
    if (sty.slant > 80 || sty.slant < -80) {
      error("silly slant `%1' degrees", sty.slant);
      sty.slant = 0;
    }
  }
  if (sbuf_len > 0) {
    if (sbuf_len < SBUF_SIZE
	&& sty == sbuf_style
	&& sbuf_vpos == env->vpos
	&& sbuf_color == *env->col) {
      if (sbuf_end_hpos == env->hpos) {
	sbuf[sbuf_len++] = code;
	sbuf_end_hpos += w + sbuf_kern;
	return;
      }
      if (sbuf_len == 1 && sbuf_kern == 0) {
	sbuf_kern = env->hpos - sbuf_end_hpos;
	sbuf_end_hpos = env->hpos + sbuf_kern + w;
	sbuf[sbuf_len++] = code;
	return;
      }
      /* If sbuf_end_hpos - sbuf_kern == env->hpos, we are better off
	 starting a new string. */
      if (sbuf_len < SBUF_SIZE - 1 && env->hpos >= sbuf_end_hpos
	  && (sbuf_kern == 0 || sbuf_end_hpos - sbuf_kern != env->hpos)) {
	if (sbuf_space_code < 0) {
	  if (f->contains(space_char_index)) {
	    sbuf_space_code = f->get_code(space_char_index);
	    sbuf_space_width = env->hpos - sbuf_end_hpos;
	    sbuf_end_hpos = env->hpos + w + sbuf_kern;
	    sbuf[sbuf_len++] = sbuf_space_code;
	    sbuf[sbuf_len++] = code;
	    sbuf_space_count++;
	    return;
	  }
	}
	else {
	  int diff = env->hpos - sbuf_end_hpos - sbuf_space_width;
	  if (diff == 0 || (equalise_spaces && (diff == 1 || diff == -1))) {
	    sbuf_end_hpos = env->hpos + w + sbuf_kern;
	    sbuf[sbuf_len++] = sbuf_space_code;
	    sbuf[sbuf_len++] = code;
	    sbuf_space_count++;
	    if (diff == 1)
	      sbuf_space_diff_count++;
	    else if (diff == -1)
	      sbuf_space_diff_count--;
	    return;
	  }
	}
      }
    }
    flush_sbuf();
  }
  sbuf_len = 1;
  sbuf[0] = code;
  sbuf_end_hpos = env->hpos + w;
  sbuf_start_hpos = env->hpos;
  sbuf_vpos = env->vpos;
  sbuf_style = sty;
  sbuf_space_code = -1;
  sbuf_space_width = 0;
  sbuf_space_count = sbuf_space_diff_count = 0;
  sbuf_kern = 0;
  if (sbuf_color != *env->col)
    set_color(env->col);
}

static char *make_encoding_name(int encoding_index)
{
  static char buf[3 + INT_DIGITS + 1];
  sprintf(buf, "ENC%d", encoding_index);
  return buf;
}

static char *make_subencoding_name(int subencoding_index)
{
  static char buf[6 + INT_DIGITS + 1];
  sprintf(buf, "SUBENC%d", subencoding_index);
  return buf;
}

const char *const WS = " \t\n\r";

void ps_printer::define_encoding(const char *encoding, int encoding_index)
{
  char *vec[256];
  int i;
  for (i = 0; i < 256; i++)
    vec[i] = 0;
  char *path;
  FILE *fp = font::open_file(encoding, &path);
  if (fp == 0)
    fatal("can't open encoding file `%1'", encoding);
  int lineno = 1;
  const int BUFFER_SIZE = 512;
  char buf[BUFFER_SIZE];
  while (fgets(buf, BUFFER_SIZE, fp) != 0) {
    char *p = buf;
    while (csspace(*p))
      p++;
    if (*p != '#' && *p != '\0' && (p = strtok(buf, WS)) != 0) {
      char *q = strtok(0, WS);
      int n = 0;		// pacify compiler
      if (q == 0 || sscanf(q, "%d", &n) != 1 || n < 0 || n >= 256)
	fatal_with_file_and_line(path, lineno, "bad second field");
      vec[n] = new char[strlen(p) + 1];
      strcpy(vec[n], p);
    }
    lineno++;
  }
  a_delete path;
  out.put_literal_symbol(make_encoding_name(encoding_index))
     .put_delimiter('[');
  for (i = 0; i < 256; i++) {
    if (vec[i] == 0)
      out.put_literal_symbol(".notdef");
    else {
      out.put_literal_symbol(vec[i]);
      a_delete vec[i];
    }
  }
  out.put_delimiter(']')
     .put_symbol("def");
  fclose(fp);
}

void ps_printer::reencode_font(ps_font *f)
{
  out.put_literal_symbol(f->reencoded_name)
     .put_symbol(make_encoding_name(f->encoding_index))
     .put_literal_symbol(f->get_internal_name())
     .put_symbol("RE");
}

void ps_printer::encode_fonts()
{
  if (next_encoding_index == 0)
    return;
  char *done_encoding = new char[next_encoding_index];
  for (int i = 0; i < next_encoding_index; i++)
    done_encoding[i] = 0;
  for (font_pointer_list *f = font_list; f; f = f->next) {
    int encoding_index = ((ps_font *)f->p)->encoding_index;
    if (encoding_index >= 0) {
      assert(encoding_index < next_encoding_index);
      if (!done_encoding[encoding_index]) {
	done_encoding[encoding_index] = 1;
	define_encoding(((ps_font *)f->p)->encoding, encoding_index);
      }
      reencode_font((ps_font *)f->p);
    }
  }
  a_delete done_encoding;
}

void ps_printer::encode_subfont(subencoding *sub)
{
  out.put_literal_symbol(make_subencoding_name(sub->idx))
     .put_delimiter('[');
  for (int i = 0; i < 256; i++)
  {
    if (sub->glyphs[i])
      out.put_literal_symbol(sub->glyphs[i]);
    else
      out.put_literal_symbol(".notdef");
  }
  out.put_delimiter(']')
     .put_symbol("def");
}

void ps_printer::set_style(const style &sty)
{
  char buf[1 + INT_DIGITS + 1];
  for (int i = 0; i < ndefined_styles; i++)
    if (sty == defined_styles[i]) {
      sprintf(buf, "F%d", i);
      out.put_symbol(buf);
      return;
    }
  if (ndefined_styles >= MAX_DEFINED_STYLES)
    ndefined_styles = 0;
  sprintf(buf, "F%d", ndefined_styles);
  out.put_literal_symbol(buf);
  const char *psname = sty.f->get_internal_name();
  if (psname == 0)
    fatal("no internalname specified for font `%1'", sty.f->get_name());
  char *encoding = ((ps_font *)sty.f)->encoding;
  if (sty.sub == 0) {
    if (encoding != 0) {
      char *s = ((ps_font *)sty.f)->reencoded_name;
      if (s == 0) {
	int ei = set_encoding_index((ps_font *)sty.f);
	char *tem = new char[strlen(psname) + 1 + INT_DIGITS + 1];
	sprintf(tem, "%s@%d", psname, ei);
	psname = tem;
	((ps_font *)sty.f)->reencoded_name = tem;
      }
      else
        psname = s;
    }
  }
  else
    psname = get_subfont(sty.sub, psname);
  out.put_fix_number((font::res/(72*font::sizescale))*sty.point_size);
  if (sty.height != 0 || sty.slant != 0) {
    int h = sty.height == 0 ? sty.point_size : sty.height;
    h *= font::res/(72*font::sizescale);
    int c = int(h*tan(radians(sty.slant)) + .5);
    out.put_fix_number(c)
       .put_fix_number(h)
       .put_literal_symbol(psname)
       .put_symbol("MF");
  }
  else {
    out.put_literal_symbol(psname)
       .put_symbol("SF");
  }
  defined_styles[ndefined_styles++] = sty;
}

void ps_printer::set_color(color *col, int fill)
{
  sbuf_color = *col;
  unsigned int components[4];
  char s[3];
  color_scheme cs = col->get_components(components);
  s[0] = fill ? 'F' : 'C';
  s[2] = 0;
  switch (cs) {
  case DEFAULT:			// black
    out.put_symbol("0");
    s[1] = 'g';
    break;
  case RGB:
    out.put_color(Red)
       .put_color(Green)
       .put_color(Blue);
    s[1] = 'r';
    break;
  case CMY:
    col->get_cmyk(&Cyan, &Magenta, &Yellow, &Black);
    // fall through
  case CMYK:
    out.put_color(Cyan)
       .put_color(Magenta)
       .put_color(Yellow)
       .put_color(Black);
    s[1] = 'k';
    cmyk_flag = 1;
    break;
  case GRAY:
    out.put_color(Gray);
    s[1] = 'g';
    break;
  }
  out.put_symbol(s);
}

void ps_printer::set_space_code(unsigned char c)
{
  out.put_literal_symbol("SC")
     .put_number(c)
     .put_symbol("def");
}

void ps_printer::end_of_line()
{
  flush_sbuf();
  // this ensures that we do an absolute motion to the beginning of a line
  output_vpos = output_hpos = -1;
}

void ps_printer::flush_sbuf()
{
  enum {
    NONE,
    RELATIVE_H,
    RELATIVE_V,
    RELATIVE_HV,
    ABSOLUTE
    } motion = NONE;
  int space_flag = 0;
  if (sbuf_len == 0)
    return;
  if (output_style != sbuf_style) {
    set_style(sbuf_style);
    output_style = sbuf_style;
  }
  int extra_space = 0;
  if (output_hpos < 0 || output_vpos < 0)
    motion = ABSOLUTE;
  else {
    if (output_hpos != sbuf_start_hpos)
      motion = RELATIVE_H;
    if (output_vpos != sbuf_vpos) {
      if  (motion != NONE)
	motion = RELATIVE_HV;
      else
	motion = RELATIVE_V;
    }
  }
  if (sbuf_space_code >= 0) {
    int w = sbuf_style.f->get_width(space_char_index, sbuf_style.point_size);
    if (w + sbuf_kern != sbuf_space_width) {
      if (sbuf_space_code != output_space_code) {
	set_space_code(sbuf_space_code);
	output_space_code = sbuf_space_code;
      }
      space_flag = 1;
      extra_space = sbuf_space_width - w - sbuf_kern;
      if (sbuf_space_diff_count > sbuf_space_count/2)
	extra_space++;
      else if (sbuf_space_diff_count < -(sbuf_space_count/2))
	extra_space--;
    }
  }
  if (space_flag)
    out.put_fix_number(extra_space);
  if (sbuf_kern != 0)
    out.put_fix_number(sbuf_kern);
  out.put_string(sbuf, sbuf_len);
  char command_array[] = {'A', 'B', 'C', 'D',
			  'E', 'F', 'G', 'H',
			  'I', 'J', 'K', 'L',
			  'M', 'N', 'O', 'P',
			  'Q', 'R', 'S', 'T'};
  char sym[2];
  sym[0] = command_array[motion*4 + space_flag + 2*(sbuf_kern != 0)];
  sym[1] = '\0';
  switch (motion) {
  case NONE:
    break;
  case ABSOLUTE:
    out.put_fix_number(sbuf_start_hpos)
       .put_fix_number(sbuf_vpos);
    break;
  case RELATIVE_H:
    out.put_fix_number(sbuf_start_hpos - output_hpos);
    break;
  case RELATIVE_V:
    out.put_fix_number(sbuf_vpos - output_vpos);
    break;
  case RELATIVE_HV:
    out.put_fix_number(sbuf_start_hpos - output_hpos)
       .put_fix_number(sbuf_vpos - output_vpos);
    break;
  default:
    assert(0);
  }
  out.put_symbol(sym);
  output_hpos = sbuf_end_hpos;
  output_vpos = sbuf_vpos;
  sbuf_len = 0;
}

void ps_printer::set_line_thickness_and_color(const environment *env)
{
  if (line_thickness < 0) {
    if (output_draw_point_size != env->size) {
      // we ought to check for overflow here
      int lw = ((font::res/(72*font::sizescale))*linewidth*env->size)/1000;
      out.put_fix_number(lw)
	 .put_symbol("LW");
      output_draw_point_size = env->size;
      output_line_thickness = -1;
    }
  }
  else {
    if (output_line_thickness != line_thickness) {
      out.put_fix_number(line_thickness)
	 .put_symbol("LW");
      output_line_thickness = line_thickness;
      output_draw_point_size = -1;
    }
  }
  if (sbuf_color != *env->col)
    set_color(env->col);
}

void ps_printer::fill_path(const environment *env)
{
  if (sbuf_color == *env->fill)
    out.put_symbol("FL");
  else
    set_color(env->fill, 1);
}

void ps_printer::draw(int code, int *p, int np, const environment *env)
{
  if (invis_count > 0)
    return;
  flush_sbuf();
  int fill_flag = 0;
  switch (code) {
  case 'C':
    fill_flag = 1;
    // fall through
  case 'c':
    // troff adds an extra argument to C
    if (np != 1 && !(code == 'C' && np == 2)) {
      error("1 argument required for circle");
      break;
    }
    out.put_fix_number(env->hpos + p[0]/2)
       .put_fix_number(env->vpos)
       .put_fix_number(p[0]/2)
       .put_symbol("DC");
    if (fill_flag)
      fill_path(env);
    else {
      set_line_thickness_and_color(env);
      out.put_symbol("ST");
    }
    break;
  case 'l':
    if (np != 2) {
      error("2 arguments required for line");
      break;
    }
    set_line_thickness_and_color(env);
    out.put_fix_number(p[0] + env->hpos)
       .put_fix_number(p[1] + env->vpos)
       .put_fix_number(env->hpos)
       .put_fix_number(env->vpos)
       .put_symbol("DL");
    break;
  case 'E':
    fill_flag = 1;
    // fall through
  case 'e':
    if (np != 2) {
      error("2 arguments required for ellipse");
      break;
    }
    out.put_fix_number(p[0])
       .put_fix_number(p[1])
       .put_fix_number(env->hpos + p[0]/2)
       .put_fix_number(env->vpos)
       .put_symbol("DE");
    if (fill_flag)
      fill_path(env);
    else {
      set_line_thickness_and_color(env);
      out.put_symbol("ST");
    }
    break;
  case 'P':
    fill_flag = 1;
    // fall through
  case 'p':
    {
      if (np & 1) {
	error("even number of arguments required for polygon");
	break;
      }
      if (np == 0) {
	error("no arguments for polygon");
	break;
      }
      out.put_fix_number(env->hpos)
	 .put_fix_number(env->vpos)
	 .put_symbol("MT");
      for (int i = 0; i < np; i += 2)
	out.put_fix_number(p[i])
	   .put_fix_number(p[i+1])
	   .put_symbol("RL");
      out.put_symbol("CL");
      if (fill_flag)
	fill_path(env);
      else {
	set_line_thickness_and_color(env);
	out.put_symbol("ST");
      }
      break;
    }
  case '~':
    {
      if (np & 1) {
	error("even number of arguments required for spline");
	break;
      }
      if (np == 0) {
	error("no arguments for spline");
	break;
      }
      out.put_fix_number(env->hpos)
	 .put_fix_number(env->vpos)
	 .put_symbol("MT");
      out.put_fix_number(p[0]/2)
	 .put_fix_number(p[1]/2)
	 .put_symbol("RL");
      /* tnum/tden should be between 0 and 1; the closer it is to 1
	 the tighter the curve will be to the guiding lines; 2/3
	 is the standard value */
      const int tnum = 2;
      const int tden = 3;
      for (int i = 0; i < np - 2; i += 2) {
	out.put_fix_number((p[i]*tnum)/(2*tden))
	   .put_fix_number((p[i + 1]*tnum)/(2*tden))
	   .put_fix_number(p[i]/2 + (p[i + 2]*(tden - tnum))/(2*tden))
	   .put_fix_number(p[i + 1]/2 + (p[i + 3]*(tden - tnum))/(2*tden))
	   .put_fix_number((p[i] - p[i]/2) + p[i + 2]/2)
	   .put_fix_number((p[i + 1] - p[i + 1]/2) + p[i + 3]/2)
	   .put_symbol("RC");
      }
      out.put_fix_number(p[np - 2] - p[np - 2]/2)
	 .put_fix_number(p[np - 1] - p[np - 1]/2)
	 .put_symbol("RL");
      set_line_thickness_and_color(env);
      out.put_symbol("ST");
    }
    break;
  case 'a':
    {
      if (np != 4) {
	error("4 arguments required for arc");
	break;
      }
      set_line_thickness_and_color(env);
      double c[2];
      if (adjust_arc_center(p, c))
	out.put_fix_number(env->hpos + int(c[0]))
	   .put_fix_number(env->vpos + int(c[1]))
	   .put_fix_number(int(sqrt(c[0]*c[0] + c[1]*c[1])))
	   .put_float(degrees(atan2(-c[1], -c[0])))
	   .put_float(degrees(atan2(p[1] + p[3] - c[1], p[0] + p[2] - c[0])))
	   .put_symbol("DA");
      else
	out.put_fix_number(p[0] + p[2] + env->hpos)
	   .put_fix_number(p[1] + p[3] + env->vpos)
	   .put_fix_number(env->hpos)
	   .put_fix_number(env->vpos)
	   .put_symbol("DL");
    }
    break;
  case 't':
    if (np == 0)
      line_thickness = -1;
    else {
      // troff gratuitously adds an extra 0
      if (np != 1 && np != 2) {
	error("0 or 1 argument required for thickness");
	break;
      }
      line_thickness = p[0];
    }
    break;
  default:
    error("unrecognised drawing command `%1'", char(code));
    break;
  }
  output_hpos = output_vpos = -1;
}

const char *ps_printer::media_name()
{
  return "Default";
}

int ps_printer::media_width()
{
  /*
   *  NOTE:
   *  Although paper size is defined as real numbers, it seems to be
   *  a common convention to round to the nearest postscript unit.
   *  For example, a4 is really 595.276 by 841.89 but we use 595 by 842.
   *
   *  This is probably a good compromise, especially since the
   *  Postscript definition specifies that media
   *  matching should be done within a tolerance of 5 units.
   */
  return int(user_paper_width ? user_paper_width*72.0 + 0.5
			      : font::paperwidth*72.0/font::res + 0.5);
}

int ps_printer::media_height()
{
  return int(user_paper_length ? user_paper_length*72.0 + 0.5
			       : paper_length*72.0/font::res + 0.5);
}

void ps_printer::media_set()
{
  /*
   *  The setpagedevice implies an erasepage and initgraphics, and
   *  must thus precede any descriptions for a particular page.
   *
   *  NOTE:
   *  This does not work with ps2pdf when there are included eps
   *  segments that contain PageSize/setpagedevice.
   *  This might be a bug in ghostscript -- must be investigated.
   *  Using setpagedevice in an .eps is really the wrong concept, anyway.
   *
   *  NOTE:
   *  For the future, this is really the place to insert other
   *  media selection features, like:
   *    MediaColor
   *    MediaPosition
   *    MediaType
   *    MediaWeight
   *    MediaClass
   *    TraySwitch
   *    ManualFeed
   *    InsertSheet
   *    Duplex
   *    Collate
   *    ProcessColorModel
   *  etc.
   */
  if (!(broken_flags & (USE_PS_ADOBE_2_0|NO_PAPERSIZE))) { 
    out.begin_comment("BeginFeature:")
       .comment_arg("*PageSize")
       .comment_arg(media_name())
       .end_comment();
    int w = media_width();
    int h = media_height();
    if (w > 0 && h > 0)
      // warning to user is done elsewhere
      fprintf(out.get_file(),
	      "<< /PageSize [ %d %d ] /ImagingBBox null >> setpagedevice\n",
	      w, h);
    out.simple_comment("EndFeature");
  }
}

void ps_printer::begin_page(int n)
{
  out.begin_comment("Page:")
     .comment_arg(i_to_a(n));
  out.comment_arg(i_to_a(++pages_output))
     .end_comment();
  output_style.f = 0;
  output_space_code = 32;
  output_draw_point_size = -1;
  output_line_thickness = -1;
  output_hpos = output_vpos = -1;
  ndefined_styles = 0;
  out.simple_comment("BeginPageSetup");

#if 0
  /*
   *  NOTE:
   *  may decide to do this once per page
   */
  media_set();
#endif

  out.put_symbol("BP")
     .simple_comment("EndPageSetup");
  if (sbuf_color != default_color)
    set_color(&sbuf_color);
}

void ps_printer::end_page(int)
{
  flush_sbuf();
  set_color(&default_color);
  out.put_symbol("EP");
  if (invis_count != 0) {
    error("missing `endinvis' command");
    invis_count = 0;
  }
}

font *ps_printer::make_font(const char *nm)
{
  return ps_font::load_ps_font(nm);
}

ps_printer::~ps_printer()
{
  out.simple_comment("Trailer")
     .put_symbol("end")
     .simple_comment("EOF");
  if (fseek(tempfp, 0L, 0) < 0)
    fatal("fseek on temporary file failed");
  fputs("%!PS-Adobe-", stdout);
  fputs((broken_flags & USE_PS_ADOBE_2_0) ? "2.0" : "3.0", stdout);
  putchar('\n');
  out.set_file(stdout);
  if (cmyk_flag)
    out.begin_comment("Extensions:")
       .comment_arg("CMYK")
       .end_comment();
  out.begin_comment("Creator:")
     .comment_arg("groff")
     .comment_arg("version")
     .comment_arg(Version_string)
     .end_comment();
  {
    fputs("%%CreationDate: ", out.get_file());
#ifdef LONG_FOR_TIME_T
    long
#else
    time_t
#endif
    t = time(0);
    fputs(ctime(&t), out.get_file());
  }
  for (font_pointer_list *f = font_list; f; f = f->next) {
    ps_font *psf = (ps_font *)(f->p);
    rm.need_font(psf->get_internal_name());
  }
  rm.print_header_comments(out);
  out.begin_comment("Pages:")
     .comment_arg(i_to_a(pages_output))
     .end_comment();
  out.begin_comment("PageOrder:")
     .comment_arg("Ascend")
     .end_comment();
  if (!(broken_flags & NO_PAPERSIZE)) {
    int w = media_width();
    int h = media_height();
    if (w > 0 && h > 0)
      fprintf(out.get_file(),
	      "%%%%DocumentMedia: %s %d %d %d %s %s\n",
	      media_name(),			// tag name of media
	      w,				// media width
	      h,				// media height
	      0,				// weight in g/m2
	      "()",				// paper color, e.g. white
	      "()"				// preprinted form type
	     );
    else {
      if (h <= 0)
	// see ps_printer::ps_printer
	warning("bad paper height, defaulting to 11i");
      if (w <= 0)
	warning("bad paper width");
    }
  }
  out.begin_comment("Orientation:")
     .comment_arg(landscape_flag ? "Landscape" : "Portrait")
     .end_comment(); 
  if (ncopies != 1) {
    out.end_line();
    fprintf(out.get_file(), "%%%%Requirements: numcopies(%d)\n", ncopies);
  }
  out.simple_comment("EndComments");
  if (!(broken_flags & NO_PAPERSIZE)) {
    /* gv works fine without this one, but it really should be there. */
    out.simple_comment("BeginDefaults");
    fprintf(out.get_file(), "%%%%PageMedia: %s\n", media_name());
    out.simple_comment("EndDefaults");
  }
  out.simple_comment("BeginProlog");
  rm.output_prolog(out);
  if (!(broken_flags & NO_SETUP_SECTION)) {
    out.simple_comment("EndProlog");
    out.simple_comment("BeginSetup");
  }
#if 1
  /*
   * Define paper (i.e., media) size for entire document here.
   * This allows ps2pdf to correctly determine page size, for instance.
   */
  media_set();
#endif
  rm.document_setup(out);
  out.put_symbol(dict_name)
     .put_symbol("begin");
  if (ndefs > 0)
    ndefs += DEFS_DICT_SPARE;
  out.put_literal_symbol(defs_dict_name)
     .put_number(ndefs + 1)
     .put_symbol("dict")
     .put_symbol("def");
  out.put_symbol(defs_dict_name)
     .put_symbol("begin");
  out.put_literal_symbol("u")
     .put_delimiter('{')
     .put_fix_number(1)
     .put_symbol("mul")
     .put_delimiter('}')
     .put_symbol("bind")
     .put_symbol("def");
  defs += '\0';
  out.special(defs.contents());
  out.put_symbol("end");
  if (ncopies != 1)
    out.put_literal_symbol("#copies")
       .put_number(ncopies)
       .put_symbol("def");
  out.put_literal_symbol("RES")
     .put_number(res)
     .put_symbol("def");
  out.put_literal_symbol("PL");
  if (guess_flag)
    out.put_symbol("PLG");
  else
    out.put_fix_number(paper_length);
  out.put_symbol("def");
  out.put_literal_symbol("LS")
     .put_symbol(landscape_flag ? "true" : "false")
     .put_symbol("def");
  if (manual_feed_flag) {
    out.begin_comment("BeginFeature:")
       .comment_arg("*ManualFeed")
       .comment_arg("True")
       .end_comment()
       .put_symbol("MANUAL")
       .simple_comment("EndFeature");
  }
  encode_fonts();
  while (subencodings) {
    subencoding *tem = subencodings;
    subencodings = subencodings->next;
    encode_subfont(tem);
    out.put_literal_symbol(tem->subfont)
       .put_symbol(make_subencoding_name(tem->idx))
       .put_literal_symbol(tem->p->get_internal_name())
       .put_symbol("RE");
    delete tem;
  }
  out.simple_comment((broken_flags & NO_SETUP_SECTION)
		     ? "EndProlog"
		     : "EndSetup");
  out.end_line();
  out.copy_file(tempfp);
  fclose(tempfp);
}

void ps_printer::special(char *arg, const environment *env, char type)
{
  if (type != 'p')
    return;
  typedef void (ps_printer::*SPECIAL_PROCP)(char *, const environment *);
  static struct {
    const char *name;
    SPECIAL_PROCP proc;
  } proc_table[] = {
    { "exec", &ps_printer::do_exec },
    { "def", &ps_printer::do_def },
    { "mdef", &ps_printer::do_mdef },
    { "import", &ps_printer::do_import },
    { "file", &ps_printer::do_file },
    { "invis", &ps_printer::do_invis },
    { "endinvis", &ps_printer::do_endinvis },
  };
  char *p;
  for (p = arg; *p == ' ' || *p == '\n'; p++)
    ;
  char *tag = p;
  for (; *p != '\0' && *p != ':' && *p != ' ' && *p != '\n'; p++)
    ;
  if (*p == '\0' || strncmp(tag, "ps", p - tag) != 0) {
    error("X command without `ps:' tag ignored");
    return;
  }
  p++;
  for (; *p == ' ' || *p == '\n'; p++)
    ;
  char *command = p;
  for (; *p != '\0' && *p != ' ' && *p != '\n'; p++)
    ;
  if (*command == '\0') {
    error("empty X command ignored");
    return;
  }
  for (unsigned int i = 0; i < sizeof(proc_table)/sizeof(proc_table[0]); i++)
    if (strncmp(command, proc_table[i].name, p - command) == 0) {
      (this->*(proc_table[i].proc))(p, env);
      return;
    }
  error("X command `%1' not recognised", command);
}

// A conforming PostScript document must not have lines longer
// than 255 characters (excluding line termination characters).

static int check_line_lengths(const char *p)
{
  for (;;) {
    const char *end = strchr(p, '\n');
    if (end == 0)
      end = strchr(p, '\0');
    if (end - p > 255)
      return 0;
    if (*end == '\0')
      break;
    p = end + 1;
  }
  return 1;
}

void ps_printer::do_exec(char *arg, const environment *env)
{
  flush_sbuf();
  while (csspace(*arg))
    arg++;
  if (*arg == '\0') {
    error("missing argument to X exec command");
    return;
  }
  if (!check_line_lengths(arg)) {
    error("lines in X exec command must not be more than 255 characters long");
    return;
  }
  out.put_fix_number(env->hpos)
     .put_fix_number(env->vpos)
     .put_symbol("EBEGIN")
     .special(arg)
     .put_symbol("EEND");
  output_hpos = output_vpos = -1;
  output_style.f = 0;
  output_draw_point_size = -1;
  output_line_thickness = -1;
  ndefined_styles = 0;
  if (!ndefs)
    ndefs = 1;
}

void ps_printer::do_file(char *arg, const environment *env)
{
  flush_sbuf();
  while (csspace(*arg))
    arg++;
  if (*arg == '\0') {
    error("missing argument to X file command");
    return;
  }
  const char *filename = arg;
  do {
    ++arg;
  } while (*arg != '\0' && *arg != ' ' && *arg != '\n');
  out.put_fix_number(env->hpos)
     .put_fix_number(env->vpos)
     .put_symbol("EBEGIN");
  rm.import_file(filename, out);
  out.put_symbol("EEND");
  output_hpos = output_vpos = -1;
  output_style.f = 0;
  output_draw_point_size = -1;
  output_line_thickness = -1;
  ndefined_styles = 0;
  if (!ndefs)
    ndefs = 1;
}

void ps_printer::do_def(char *arg, const environment *)
{
  flush_sbuf();
  while (csspace(*arg))
    arg++;
  if (!check_line_lengths(arg)) {
    error("lines in X def command must not be more than 255 characters long");
    return;
  }
  defs += arg;
  if (*arg != '\0' && strchr(arg, '\0')[-1] != '\n')
    defs += '\n';
  ndefs++;
}

// Like def, but the first argument says how many definitions it contains.

void ps_printer::do_mdef(char *arg, const environment *)
{
  flush_sbuf();
  char *p;
  int n = (int)strtol(arg, &p, 10);
  if (n == 0 && p == arg) {
    error("first argument to X mdef must be an integer");
    return;
  }
  if (n < 0) {
    error("out of range argument `%1' to X mdef command", int(n));
    return;
  }
  arg = p;
  while (csspace(*arg))
    arg++;
  if (!check_line_lengths(arg)) {
    error("lines in X mdef command must not be more than 255 characters long");
    return;
  }
  defs += arg;
  if (*arg != '\0' && strchr(arg, '\0')[-1] != '\n')
    defs += '\n';
  ndefs += n;
}

void ps_printer::do_import(char *arg, const environment *env)
{
  flush_sbuf();
  while (*arg == ' ' || *arg == '\n')
    arg++;
  char *p;
  for (p = arg; *p != '\0' && *p != ' ' && *p != '\n'; p++)
    ;
  if (*p != '\0')
    *p++ = '\0';
  int parms[6];
  int nparms = 0;
  while (nparms < 6) {
    char *end;
    long n = strtol(p, &end, 10);
    if (n == 0 && end == p)
      break;
    parms[nparms++] = int(n);
    p = end;
  }
  if (csalpha(*p) && (p[1] == '\0' || p[1] == ' ' || p[1] == '\n')) {
    error("scaling indicators not allowed in arguments for X import command");
    return;
  }
  while (*p == ' ' || *p == '\n')
    p++;
  if (nparms < 5) {
    if (*p == '\0')
      error("too few arguments for X import command");
    else
      error("invalid argument `%1' for X import command", p);
    return;
  }
  if (*p != '\0') {
    error("superfluous argument `%1' for X import command", p);
    return;
  }
  int llx = parms[0];
  int lly = parms[1];
  int urx = parms[2];
  int ury = parms[3];
  int desired_width = parms[4];
  int desired_height = parms[5];
  if (desired_width <= 0) {
    error("bad width argument `%1' for X import command: must be > 0",
	  desired_width);
    return;
  }
  if (nparms == 6 && desired_height <= 0) {
    error("bad height argument `%1' for X import command: must be > 0",
	  desired_height);
    return;
  }
  if (llx == urx) {
    error("llx and urx arguments for X import command must not be equal");
    return;
  }
  if (lly == ury) {
    error("lly and ury arguments for X import command must not be equal");
    return;
  }
  if (nparms == 5) {
    int old_wid = urx - llx;
    int old_ht = ury - lly;
    if (old_wid < 0)
      old_wid = -old_wid;
    if (old_ht < 0)
      old_ht = -old_ht;
    desired_height = int(desired_width*(double(old_ht)/double(old_wid)) + .5);
  }
  if (env->vpos - desired_height < 0)
    warning("top of imported graphic is above the top of the page");
  out.put_number(llx)
     .put_number(lly)
     .put_fix_number(desired_width)
     .put_number(urx - llx)
     .put_fix_number(-desired_height)
     .put_number(ury - lly)
     .put_fix_number(env->hpos)
     .put_fix_number(env->vpos)
     .put_symbol("PBEGIN");
  rm.import_file(arg, out);
  // do this here just in case application defines PEND
  out.put_symbol("end")
     .put_symbol("PEND");
}

void ps_printer::do_invis(char *, const environment *)
{
  invis_count++;
}

void ps_printer::do_endinvis(char *, const environment *)
{
  if (invis_count == 0)
    error("unbalanced `endinvis' command");
  else
    --invis_count;
}

printer *make_printer()
{
  return new ps_printer(user_paper_length);
}

static void usage(FILE *stream);

int main(int argc, char **argv)
{
  setlocale(LC_NUMERIC, "C");
  program_name = argv[0];
  string env;
  static char stderr_buf[BUFSIZ];
  setbuf(stderr, stderr_buf);
  int c;
  static const struct option long_options[] = {
    { "help", no_argument, 0, CHAR_MAX + 1 },
    { "version", no_argument, 0, 'v' },
    { NULL, 0, 0, 0 }
  };
  while ((c = getopt_long(argc, argv, "b:c:F:gI:lmp:P:vw:", long_options, NULL))
	 != EOF)
    switch(c) {
    case 'b':
      // XXX check this
      broken_flags = atoi(optarg);
      bflag = 1;
      break;
    case 'c':
      if (sscanf(optarg, "%d", &ncopies) != 1 || ncopies <= 0) {
	error("bad number of copies `%s'", optarg);
	ncopies = 1;
      }
      break;
    case 'F':
      font::command_line_font_dir(optarg);
      break;
    case 'g':
      guess_flag = 1;
      break;
    case 'I':
      include_search_path.command_line_dir(optarg);
      break;
    case 'l':
      landscape_flag = 1;
      break;
    case 'm':
      manual_feed_flag = 1;
      break;
    case 'p':
      if (!font::scan_papersize(optarg, 0,
				&user_paper_length, &user_paper_width))
	error("invalid custom paper size `%1' ignored", optarg);
      break;
    case 'P':
      env = "GROPS_PROLOGUE";
      env += '=';
      env += optarg;
      env += '\0';
      if (putenv(strsave(env.contents())))
	fatal("putenv failed");
      break;
    case 'v':
      printf("GNU grops (groff) version %s\n", Version_string);
      exit(0);
      break;
    case 'w':
      if (sscanf(optarg, "%d", &linewidth) != 1 || linewidth < 0) {
	error("bad linewidth `%1'", optarg);
	linewidth = -1;
      }
      break;
    case CHAR_MAX + 1: // --help
      usage(stdout);
      exit(0);
      break;
    case '?':
      usage(stderr);
      exit(1);
      break;
    default:
      assert(0);
    }
  font::set_unknown_desc_command_handler(handle_unknown_desc_command);
  SET_BINARY(fileno(stdout));
  if (optind >= argc)
    do_file("-");
  else {
    for (int i = optind; i < argc; i++)
      do_file(argv[i]);
  }
  return 0;
}

static void usage(FILE *stream)
{
  fprintf(stream,
"usage: %s [-glmv] [-b n] [-c n] [-w n] [-I dir] [-P prologue]\n"
"       [-F dir] [files ...]\n",
    program_name);
}
