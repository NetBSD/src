// script.cc -- handle linker scripts for gold.

// Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#include "gold.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fnmatch.h>
#include <string>
#include <vector>
#include "filenames.h"

#include "elfcpp.h"
#include "demangle.h"
#include "dirsearch.h"
#include "options.h"
#include "fileread.h"
#include "workqueue.h"
#include "readsyms.h"
#include "parameters.h"
#include "layout.h"
#include "symtab.h"
#include "script.h"
#include "script-c.h"

namespace gold
{

// A token read from a script file.  We don't implement keywords here;
// all keywords are simply represented as a string.

class Token
{
 public:
  // Token classification.
  enum Classification
  {
    // Token is invalid.
    TOKEN_INVALID,
    // Token indicates end of input.
    TOKEN_EOF,
    // Token is a string of characters.
    TOKEN_STRING,
    // Token is a quoted string of characters.
    TOKEN_QUOTED_STRING,
    // Token is an operator.
    TOKEN_OPERATOR,
    // Token is a number (an integer).
    TOKEN_INTEGER
  };

  // We need an empty constructor so that we can put this STL objects.
  Token()
    : classification_(TOKEN_INVALID), value_(NULL), value_length_(0),
      opcode_(0), lineno_(0), charpos_(0)
  { }

  // A general token with no value.
  Token(Classification classification, int lineno, int charpos)
    : classification_(classification), value_(NULL), value_length_(0),
      opcode_(0), lineno_(lineno), charpos_(charpos)
  {
    gold_assert(classification == TOKEN_INVALID
		|| classification == TOKEN_EOF);
  }

  // A general token with a value.
  Token(Classification classification, const char* value, size_t length,
	int lineno, int charpos)
    : classification_(classification), value_(value), value_length_(length),
      opcode_(0), lineno_(lineno), charpos_(charpos)
  {
    gold_assert(classification != TOKEN_INVALID
		&& classification != TOKEN_EOF);
  }

  // A token representing an operator.
  Token(int opcode, int lineno, int charpos)
    : classification_(TOKEN_OPERATOR), value_(NULL), value_length_(0),
      opcode_(opcode), lineno_(lineno), charpos_(charpos)
  { }

  // Return whether the token is invalid.
  bool
  is_invalid() const
  { return this->classification_ == TOKEN_INVALID; }

  // Return whether this is an EOF token.
  bool
  is_eof() const
  { return this->classification_ == TOKEN_EOF; }

  // Return the token classification.
  Classification
  classification() const
  { return this->classification_; }

  // Return the line number at which the token starts.
  int
  lineno() const
  { return this->lineno_; }

  // Return the character position at this the token starts.
  int
  charpos() const
  { return this->charpos_; }

  // Get the value of a token.

  const char*
  string_value(size_t* length) const
  {
    gold_assert(this->classification_ == TOKEN_STRING
		|| this->classification_ == TOKEN_QUOTED_STRING);
    *length = this->value_length_;
    return this->value_;
  }

  int
  operator_value() const
  {
    gold_assert(this->classification_ == TOKEN_OPERATOR);
    return this->opcode_;
  }

  uint64_t
  integer_value() const
  {
    gold_assert(this->classification_ == TOKEN_INTEGER);
    // Null terminate.
    std::string s(this->value_, this->value_length_);
    return strtoull(s.c_str(), NULL, 0);
  }

 private:
  // The token classification.
  Classification classification_;
  // The token value, for TOKEN_STRING or TOKEN_QUOTED_STRING or
  // TOKEN_INTEGER.
  const char* value_;
  // The length of the token value.
  size_t value_length_;
  // The token value, for TOKEN_OPERATOR.
  int opcode_;
  // The line number where this token started (one based).
  int lineno_;
  // The character position within the line where this token started
  // (one based).
  int charpos_;
};

// This class handles lexing a file into a sequence of tokens.

class Lex
{
 public:
  // We unfortunately have to support different lexing modes, because
  // when reading different parts of a linker script we need to parse
  // things differently.
  enum Mode
  {
    // Reading an ordinary linker script.
    LINKER_SCRIPT,
    // Reading an expression in a linker script.
    EXPRESSION,
    // Reading a version script.
    VERSION_SCRIPT
  };

  Lex(const char* input_string, size_t input_length, int parsing_token)
    : input_string_(input_string), input_length_(input_length),
      current_(input_string), mode_(LINKER_SCRIPT),
      first_token_(parsing_token), token_(),
      lineno_(1), linestart_(input_string)
  { }

  // Read a file into a string.
  static void
  read_file(Input_file*, std::string*);

  // Return the next token.
  const Token*
  next_token();

  // Return the current lexing mode.
  Lex::Mode
  mode() const
  { return this->mode_; }

  // Set the lexing mode.
  void
  set_mode(Mode mode)
  { this->mode_ = mode; }

 private:
  Lex(const Lex&);
  Lex& operator=(const Lex&);

  // Make a general token with no value at the current location.
  Token
  make_token(Token::Classification c, const char* start) const
  { return Token(c, this->lineno_, start - this->linestart_ + 1); }

  // Make a general token with a value at the current location.
  Token
  make_token(Token::Classification c, const char* v, size_t len,
	     const char* start)
    const
  { return Token(c, v, len, this->lineno_, start - this->linestart_ + 1); }

  // Make an operator token at the current location.
  Token
  make_token(int opcode, const char* start) const
  { return Token(opcode, this->lineno_, start - this->linestart_ + 1); }

  // Make an invalid token at the current location.
  Token
  make_invalid_token(const char* start)
  { return this->make_token(Token::TOKEN_INVALID, start); }

  // Make an EOF token at the current location.
  Token
  make_eof_token(const char* start)
  { return this->make_token(Token::TOKEN_EOF, start); }

  // Return whether C can be the first character in a name.  C2 is the
  // next character, since we sometimes need that.
  inline bool
  can_start_name(char c, char c2);

  // If C can appear in a name which has already started, return a
  // pointer to a character later in the token or just past
  // it. Otherwise, return NULL.
  inline const char*
  can_continue_name(const char* c);

  // Return whether C, C2, C3 can start a hex number.
  inline bool
  can_start_hex(char c, char c2, char c3);

  // If C can appear in a hex number which has already started, return
  // a pointer to a character later in the token or just past
  // it. Otherwise, return NULL.
  inline const char*
  can_continue_hex(const char* c);

  // Return whether C can start a non-hex number.
  static inline bool
  can_start_number(char c);

  // If C can appear in a decimal number which has already started,
  // return a pointer to a character later in the token or just past
  // it. Otherwise, return NULL.
  inline const char*
  can_continue_number(const char* c)
  { return Lex::can_start_number(*c) ? c + 1 : NULL; }

  // If C1 C2 C3 form a valid three character operator, return the
  // opcode.  Otherwise return 0.
  static inline int
  three_char_operator(char c1, char c2, char c3);

  // If C1 C2 form a valid two character operator, return the opcode.
  // Otherwise return 0.
  static inline int
  two_char_operator(char c1, char c2);

  // If C1 is a valid one character operator, return the opcode.
  // Otherwise return 0.
  static inline int
  one_char_operator(char c1);

  // Read the next token.
  Token
  get_token(const char**);

  // Skip a C style /* */ comment.  Return false if the comment did
  // not end.
  bool
  skip_c_comment(const char**);

  // Skip a line # comment.  Return false if there was no newline.
  bool
  skip_line_comment(const char**);

  // Build a token CLASSIFICATION from all characters that match
  // CAN_CONTINUE_FN.  The token starts at START.  Start matching from
  // MATCH.  Set *PP to the character following the token.
  inline Token
  gather_token(Token::Classification,
	       const char* (Lex::*can_continue_fn)(const char*),
	       const char* start, const char* match, const char** pp);

  // Build a token from a quoted string.
  Token
  gather_quoted_string(const char** pp);

  // The string we are tokenizing.
  const char* input_string_;
  // The length of the string.
  size_t input_length_;
  // The current offset into the string.
  const char* current_;
  // The current lexing mode.
  Mode mode_;
  // The code to use for the first token.  This is set to 0 after it
  // is used.
  int first_token_;
  // The current token.
  Token token_;
  // The current line number.
  int lineno_;
  // The start of the current line in the string.
  const char* linestart_;
};

// Read the whole file into memory.  We don't expect linker scripts to
// be large, so we just use a std::string as a buffer.  We ignore the
// data we've already read, so that we read aligned buffers.

void
Lex::read_file(Input_file* input_file, std::string* contents)
{
  off_t filesize = input_file->file().filesize();
  contents->clear();
  contents->reserve(filesize);

  off_t off = 0;
  unsigned char buf[BUFSIZ];
  while (off < filesize)
    {
      off_t get = BUFSIZ;
      if (get > filesize - off)
	get = filesize - off;
      input_file->file().read(off, get, buf);
      contents->append(reinterpret_cast<char*>(&buf[0]), get);
      off += get;
    }
}

// Return whether C can be the start of a name, if the next character
// is C2.  A name can being with a letter, underscore, period, or
// dollar sign.  Because a name can be a file name, we also permit
// forward slash, backslash, and tilde.  Tilde is the tricky case
// here; GNU ld also uses it as a bitwise not operator.  It is only
// recognized as the operator if it is not immediately followed by
// some character which can appear in a symbol.  That is, when we
// don't know that we are looking at an expression, "~0" is a file
// name, and "~ 0" is an expression using bitwise not.  We are
// compatible.

inline bool
Lex::can_start_name(char c, char c2)
{
  switch (c)
    {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'Q': case 'P': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'q': case 'p': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
    case '_': case '.': case '$':
      return true;

    case '/': case '\\':
      return this->mode_ == LINKER_SCRIPT;

    case '~':
      return this->mode_ == LINKER_SCRIPT && can_continue_name(&c2);

    case '*': case '[': 
      return (this->mode_ == VERSION_SCRIPT
	      || (this->mode_ == LINKER_SCRIPT
		  && can_continue_name(&c2)));

    default:
      return false;
    }
}

// Return whether C can continue a name which has already started.
// Subsequent characters in a name are the same as the leading
// characters, plus digits and "=+-:[],?*".  So in general the linker
// script language requires spaces around operators, unless we know
// that we are parsing an expression.

inline const char*
Lex::can_continue_name(const char* c)
{
  switch (*c)
    {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'Q': case 'P': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'q': case 'p': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
    case '_': case '.': case '$':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return c + 1;

    case '/': case '\\': case '~':
    case '=': case '+':
    case ',':
      if (this->mode_ == LINKER_SCRIPT)
        return c + 1;
      return NULL;

    case '[': case ']': case '*': case '?': case '-':
      if (this->mode_ == LINKER_SCRIPT || this->mode_ == VERSION_SCRIPT)
        return c + 1;
      return NULL;

    case '^':
      if (this->mode_ == VERSION_SCRIPT)
        return c + 1;
      return NULL;

    case ':':
      if (this->mode_ == LINKER_SCRIPT)
        return c + 1;
      else if (this->mode_ == VERSION_SCRIPT && (c[1] == ':'))
        {
          // A name can have '::' in it, as that's a c++ namespace
          // separator. But a single colon is not part of a name.
          return c + 2;
        }
      return NULL;

    default:
      return NULL;
    }
}

// For a number we accept 0x followed by hex digits, or any sequence
// of digits.  The old linker accepts leading '$' for hex, and
// trailing HXBOD.  Those are for MRI compatibility and we don't
// accept them.  The old linker also accepts trailing MK for mega or
// kilo.  FIXME: Those are mentioned in the documentation, and we
// should accept them.

// Return whether C1 C2 C3 can start a hex number.

inline bool
Lex::can_start_hex(char c1, char c2, char c3)
{
  if (c1 == '0' && (c2 == 'x' || c2 == 'X'))
    return this->can_continue_hex(&c3);
  return false;
}

// Return whether C can appear in a hex number.

inline const char*
Lex::can_continue_hex(const char* c)
{
  switch (*c)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      return c + 1;

    default:
      return NULL;
    }
}

// Return whether C can start a non-hex number.

inline bool
Lex::can_start_number(char c)
{
  switch (c)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return true;

    default:
      return false;
    }
}

// If C1 C2 C3 form a valid three character operator, return the
// opcode (defined in the yyscript.h file generated from yyscript.y).
// Otherwise return 0.

inline int
Lex::three_char_operator(char c1, char c2, char c3)
{
  switch (c1)
    {
    case '<':
      if (c2 == '<' && c3 == '=')
	return LSHIFTEQ;
      break;
    case '>':
      if (c2 == '>' && c3 == '=')
	return RSHIFTEQ;
      break;
    default:
      break;
    }
  return 0;
}

// If C1 C2 form a valid two character operator, return the opcode
// (defined in the yyscript.h file generated from yyscript.y).
// Otherwise return 0.

inline int
Lex::two_char_operator(char c1, char c2)
{
  switch (c1)
    {
    case '=':
      if (c2 == '=')
	return EQ;
      break;
    case '!':
      if (c2 == '=')
	return NE;
      break;
    case '+':
      if (c2 == '=')
	return PLUSEQ;
      break;
    case '-':
      if (c2 == '=')
	return MINUSEQ;
      break;
    case '*':
      if (c2 == '=')
	return MULTEQ;
      break;
    case '/':
      if (c2 == '=')
	return DIVEQ;
      break;
    case '|':
      if (c2 == '=')
	return OREQ;
      if (c2 == '|')
	return OROR;
      break;
    case '&':
      if (c2 == '=')
	return ANDEQ;
      if (c2 == '&')
	return ANDAND;
      break;
    case '>':
      if (c2 == '=')
	return GE;
      if (c2 == '>')
	return RSHIFT;
      break;
    case '<':
      if (c2 == '=')
	return LE;
      if (c2 == '<')
	return LSHIFT;
      break;
    default:
      break;
    }
  return 0;
}

// If C1 is a valid operator, return the opcode.  Otherwise return 0.

inline int
Lex::one_char_operator(char c1)
{
  switch (c1)
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '!':
    case '&':
    case '|':
    case '^':
    case '~':
    case '<':
    case '>':
    case '=':
    case '?':
    case ',':
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case ':':
    case ';':
      return c1;
    default:
      return 0;
    }
}

// Skip a C style comment.  *PP points to just after the "/*".  Return
// false if the comment did not end.

bool
Lex::skip_c_comment(const char** pp)
{
  const char* p = *pp;
  while (p[0] != '*' || p[1] != '/')
    {
      if (*p == '\0')
	{
	  *pp = p;
	  return false;
	}

      if (*p == '\n')
	{
	  ++this->lineno_;
	  this->linestart_ = p + 1;
	}
      ++p;
    }

  *pp = p + 2;
  return true;
}

// Skip a line # comment.  Return false if there was no newline.

bool
Lex::skip_line_comment(const char** pp)
{
  const char* p = *pp;
  size_t skip = strcspn(p, "\n");
  if (p[skip] == '\0')
    {
      *pp = p + skip;
      return false;
    }

  p += skip + 1;
  ++this->lineno_;
  this->linestart_ = p;
  *pp = p;

  return true;
}

// Build a token CLASSIFICATION from all characters that match
// CAN_CONTINUE_FN.  Update *PP.

inline Token
Lex::gather_token(Token::Classification classification,
		  const char* (Lex::*can_continue_fn)(const char*),
		  const char* start,
		  const char* match,
		  const char **pp)
{
  const char* new_match = NULL;
  while ((new_match = (this->*can_continue_fn)(match)))
    match = new_match;
  *pp = match;
  return this->make_token(classification, start, match - start, start);
}

// Build a token from a quoted string.

Token
Lex::gather_quoted_string(const char** pp)
{
  const char* start = *pp;
  const char* p = start;
  ++p;
  size_t skip = strcspn(p, "\"\n");
  if (p[skip] != '"')
    return this->make_invalid_token(start);
  *pp = p + skip + 1;
  return this->make_token(Token::TOKEN_QUOTED_STRING, p, skip, start);
}

// Return the next token at *PP.  Update *PP.  General guideline: we
// require linker scripts to be simple ASCII.  No unicode linker
// scripts.  In particular we can assume that any '\0' is the end of
// the input.

Token
Lex::get_token(const char** pp)
{
  const char* p = *pp;

  while (true)
    {
      if (*p == '\0')
	{
	  *pp = p;
	  return this->make_eof_token(p);
	}

      // Skip whitespace quickly.
      while (*p == ' ' || *p == '\t')
	++p;

      if (*p == '\n')
	{
	  ++p;
	  ++this->lineno_;
	  this->linestart_ = p;
	  continue;
	}

      // Skip C style comments.
      if (p[0] == '/' && p[1] == '*')
	{
	  int lineno = this->lineno_;
	  int charpos = p - this->linestart_ + 1;

	  *pp = p + 2;
	  if (!this->skip_c_comment(pp))
	    return Token(Token::TOKEN_INVALID, lineno, charpos);
	  p = *pp;

	  continue;
	}

      // Skip line comments.
      if (*p == '#')
	{
	  *pp = p + 1;
	  if (!this->skip_line_comment(pp))
	    return this->make_eof_token(p);
	  p = *pp;
	  continue;
	}

      // Check for a name.
      if (this->can_start_name(p[0], p[1]))
	return this->gather_token(Token::TOKEN_STRING,
				  &Lex::can_continue_name,
				  p, p + 1, pp);

      // We accept any arbitrary name in double quotes, as long as it
      // does not cross a line boundary.
      if (*p == '"')
	{
	  *pp = p;
	  return this->gather_quoted_string(pp);
	}

      // Check for a number.

      if (this->can_start_hex(p[0], p[1], p[2]))
	return this->gather_token(Token::TOKEN_INTEGER,
				  &Lex::can_continue_hex,
				  p, p + 3, pp);

      if (Lex::can_start_number(p[0]))
	return this->gather_token(Token::TOKEN_INTEGER,
				  &Lex::can_continue_number,
				  p, p + 1, pp);

      // Check for operators.

      int opcode = Lex::three_char_operator(p[0], p[1], p[2]);
      if (opcode != 0)
	{
	  *pp = p + 3;
	  return this->make_token(opcode, p);
	}

      opcode = Lex::two_char_operator(p[0], p[1]);
      if (opcode != 0)
	{
	  *pp = p + 2;
	  return this->make_token(opcode, p);
	}

      opcode = Lex::one_char_operator(p[0]);
      if (opcode != 0)
	{
	  *pp = p + 1;
	  return this->make_token(opcode, p);
	}

      return this->make_token(Token::TOKEN_INVALID, p);
    }
}

// Return the next token.

const Token*
Lex::next_token()
{
  // The first token is special.
  if (this->first_token_ != 0)
    {
      this->token_ = Token(this->first_token_, 0, 0);
      this->first_token_ = 0;
      return &this->token_;
    }

  this->token_ = this->get_token(&this->current_);

  // Don't let an early null byte fool us into thinking that we've
  // reached the end of the file.
  if (this->token_.is_eof()
      && (static_cast<size_t>(this->current_ - this->input_string_)
	  < this->input_length_))
    this->token_ = this->make_invalid_token(this->current_);

  return &this->token_;
}

// class Symbol_assignment.

// Add the symbol to the symbol table.  This makes sure the symbol is
// there and defined.  The actual value is stored later.  We can't
// determine the actual value at this point, because we can't
// necessarily evaluate the expression until all ordinary symbols have
// been finalized.

// The GNU linker lets symbol assignments in the linker script
// silently override defined symbols in object files.  We are
// compatible.  FIXME: Should we issue a warning?

void
Symbol_assignment::add_to_table(Symbol_table* symtab)
{
  elfcpp::STV vis = this->hidden_ ? elfcpp::STV_HIDDEN : elfcpp::STV_DEFAULT;
  this->sym_ = symtab->define_as_constant(this->name_.c_str(),
					  NULL, // version
					  0, // value
					  0, // size
					  elfcpp::STT_NOTYPE,
					  elfcpp::STB_GLOBAL,
					  vis,
					  0, // nonvis
					  this->provide_,
                                          true); // force_override
}

// Finalize a symbol value.

void
Symbol_assignment::finalize(Symbol_table* symtab, const Layout* layout)
{
  this->finalize_maybe_dot(symtab, layout, false, 0, NULL);
}

// Finalize a symbol value which can refer to the dot symbol.

void
Symbol_assignment::finalize_with_dot(Symbol_table* symtab,
				     const Layout* layout,
				     uint64_t dot_value,
				     Output_section* dot_section)
{
  this->finalize_maybe_dot(symtab, layout, true, dot_value, dot_section);
}

// Finalize a symbol value, internal version.

void
Symbol_assignment::finalize_maybe_dot(Symbol_table* symtab,
				      const Layout* layout,
				      bool is_dot_available,
				      uint64_t dot_value,
				      Output_section* dot_section)
{
  // If we were only supposed to provide this symbol, the sym_ field
  // will be NULL if the symbol was not referenced.
  if (this->sym_ == NULL)
    {
      gold_assert(this->provide_);
      return;
    }

  if (parameters->target().get_size() == 32)
    {
#if defined(HAVE_TARGET_32_LITTLE) || defined(HAVE_TARGET_32_BIG)
      this->sized_finalize<32>(symtab, layout, is_dot_available, dot_value,
			       dot_section);
#else
      gold_unreachable();
#endif
    }
  else if (parameters->target().get_size() == 64)
    {
#if defined(HAVE_TARGET_64_LITTLE) || defined(HAVE_TARGET_64_BIG)
      this->sized_finalize<64>(symtab, layout, is_dot_available, dot_value,
			       dot_section);
#else
      gold_unreachable();
#endif
    }
  else
    gold_unreachable();
}

template<int size>
void
Symbol_assignment::sized_finalize(Symbol_table* symtab, const Layout* layout,
				  bool is_dot_available, uint64_t dot_value,
				  Output_section* dot_section)
{
  Output_section* section;
  uint64_t final_val = this->val_->eval_maybe_dot(symtab, layout, true,
						  is_dot_available,
						  dot_value, dot_section,
						  &section);
  Sized_symbol<size>* ssym = symtab->get_sized_symbol<size>(this->sym_);
  ssym->set_value(final_val);
  if (section != NULL)
    ssym->set_output_section(section);
}

// Set the symbol value if the expression yields an absolute value.

void
Symbol_assignment::set_if_absolute(Symbol_table* symtab, const Layout* layout,
				   bool is_dot_available, uint64_t dot_value)
{
  if (this->sym_ == NULL)
    return;

  Output_section* val_section;
  uint64_t val = this->val_->eval_maybe_dot(symtab, layout, false,
					    is_dot_available, dot_value,
					    NULL, &val_section);
  if (val_section != NULL)
    return;

  if (parameters->target().get_size() == 32)
    {
#if defined(HAVE_TARGET_32_LITTLE) || defined(HAVE_TARGET_32_BIG)
      Sized_symbol<32>* ssym = symtab->get_sized_symbol<32>(this->sym_);
      ssym->set_value(val);
#else
      gold_unreachable();
#endif
    }
  else if (parameters->target().get_size() == 64)
    {
#if defined(HAVE_TARGET_64_LITTLE) || defined(HAVE_TARGET_64_BIG)
      Sized_symbol<64>* ssym = symtab->get_sized_symbol<64>(this->sym_);
      ssym->set_value(val);
#else
      gold_unreachable();
#endif
    }
  else
    gold_unreachable();
}

// Print for debugging.

void
Symbol_assignment::print(FILE* f) const
{
  if (this->provide_ && this->hidden_)
    fprintf(f, "PROVIDE_HIDDEN(");
  else if (this->provide_)
    fprintf(f, "PROVIDE(");
  else if (this->hidden_)
    gold_unreachable();

  fprintf(f, "%s = ", this->name_.c_str());
  this->val_->print(f);

  if (this->provide_ || this->hidden_)
    fprintf(f, ")");

  fprintf(f, "\n");
}

// Class Script_assertion.

// Check the assertion.

void
Script_assertion::check(const Symbol_table* symtab, const Layout* layout)
{
  if (!this->check_->eval(symtab, layout, true))
    gold_error("%s", this->message_.c_str());
}

// Print for debugging.

void
Script_assertion::print(FILE* f) const
{
  fprintf(f, "ASSERT(");
  this->check_->print(f);
  fprintf(f, ", \"%s\")\n", this->message_.c_str());
}

// Class Script_options.

Script_options::Script_options()
  : entry_(), symbol_assignments_(), version_script_info_(),
    script_sections_()
{
}

// Add a symbol to be defined.

void
Script_options::add_symbol_assignment(const char* name, size_t length,
				      Expression* value, bool provide,
				      bool hidden)
{
  if (length != 1 || name[0] != '.')
    {
      if (this->script_sections_.in_sections_clause())
	this->script_sections_.add_symbol_assignment(name, length, value,
						     provide, hidden);
      else
	{
	  Symbol_assignment* p = new Symbol_assignment(name, length, value,
						       provide, hidden);
	  this->symbol_assignments_.push_back(p);
	}
    }
  else
    {
      if (provide || hidden)
	gold_error(_("invalid use of PROVIDE for dot symbol"));
      if (!this->script_sections_.in_sections_clause())
	gold_error(_("invalid assignment to dot outside of SECTIONS"));
      else
	this->script_sections_.add_dot_assignment(value);
    }
}

// Add an assertion.

void
Script_options::add_assertion(Expression* check, const char* message,
			      size_t messagelen)
{
  if (this->script_sections_.in_sections_clause())
    this->script_sections_.add_assertion(check, message, messagelen);
  else
    {
      Script_assertion* p = new Script_assertion(check, message, messagelen);
      this->assertions_.push_back(p);
    }
}

// Create sections required by any linker scripts.

void
Script_options::create_script_sections(Layout* layout)
{
  if (this->saw_sections_clause())
    this->script_sections_.create_sections(layout);
}

// Add any symbols we are defining to the symbol table.

void
Script_options::add_symbols_to_table(Symbol_table* symtab)
{
  for (Symbol_assignments::iterator p = this->symbol_assignments_.begin();
       p != this->symbol_assignments_.end();
       ++p)
    (*p)->add_to_table(symtab);
  this->script_sections_.add_symbols_to_table(symtab);
}

// Finalize symbol values.  Also check assertions.

void
Script_options::finalize_symbols(Symbol_table* symtab, const Layout* layout)
{
  // We finalize the symbols defined in SECTIONS first, because they
  // are the ones which may have changed.  This way if symbol outside
  // SECTIONS are defined in terms of symbols inside SECTIONS, they
  // will get the right value.
  this->script_sections_.finalize_symbols(symtab, layout);

  for (Symbol_assignments::iterator p = this->symbol_assignments_.begin();
       p != this->symbol_assignments_.end();
       ++p)
    (*p)->finalize(symtab, layout);

  for (Assertions::iterator p = this->assertions_.begin();
       p != this->assertions_.end();
       ++p)
    (*p)->check(symtab, layout);
}

// Set section addresses.  We set all the symbols which have absolute
// values.  Then we let the SECTIONS clause do its thing.  This
// returns the segment which holds the file header and segment
// headers, if any.

Output_segment*
Script_options::set_section_addresses(Symbol_table* symtab, Layout* layout)
{
  for (Symbol_assignments::iterator p = this->symbol_assignments_.begin();
       p != this->symbol_assignments_.end();
       ++p)
    (*p)->set_if_absolute(symtab, layout, false, 0);

  return this->script_sections_.set_section_addresses(symtab, layout);
}

// This class holds data passed through the parser to the lexer and to
// the parser support functions.  This avoids global variables.  We
// can't use global variables because we need not be called by a
// singleton thread.

class Parser_closure
{
 public:
  Parser_closure(const char* filename,
		 const Position_dependent_options& posdep_options,
		 bool in_group, bool is_in_sysroot,
                 Command_line* command_line,
		 Script_options* script_options,
		 Lex* lex)
    : filename_(filename), posdep_options_(posdep_options),
      in_group_(in_group), is_in_sysroot_(is_in_sysroot),
      command_line_(command_line), script_options_(script_options),
      version_script_info_(script_options->version_script_info()),
      lex_(lex), lineno_(0), charpos_(0), lex_mode_stack_(), inputs_(NULL)
  { 
    // We start out processing C symbols in the default lex mode.
    language_stack_.push_back("");
    lex_mode_stack_.push_back(lex->mode());
  }

  // Return the file name.
  const char*
  filename() const
  { return this->filename_; }

  // Return the position dependent options.  The caller may modify
  // this.
  Position_dependent_options&
  position_dependent_options()
  { return this->posdep_options_; }

  // Return whether this script is being run in a group.
  bool
  in_group() const
  { return this->in_group_; }

  // Return whether this script was found using a directory in the
  // sysroot.
  bool
  is_in_sysroot() const
  { return this->is_in_sysroot_; }

  // Returns the Command_line structure passed in at constructor time.
  // This value may be NULL.  The caller may modify this, which modifies
  // the passed-in Command_line object (not a copy).
  Command_line*
  command_line()
  { return this->command_line_; }

  // Return the options which may be set by a script.
  Script_options*
  script_options()
  { return this->script_options_; }

  // Return the object in which version script information should be stored.
  Version_script_info*
  version_script()
  { return this->version_script_info_; }

  // Return the next token, and advance.
  const Token*
  next_token()
  {
    const Token* token = this->lex_->next_token();
    this->lineno_ = token->lineno();
    this->charpos_ = token->charpos();
    return token;
  }

  // Set a new lexer mode, pushing the current one.
  void
  push_lex_mode(Lex::Mode mode)
  {
    this->lex_mode_stack_.push_back(this->lex_->mode());
    this->lex_->set_mode(mode);
  }

  // Pop the lexer mode.
  void
  pop_lex_mode()
  {
    gold_assert(!this->lex_mode_stack_.empty());
    this->lex_->set_mode(this->lex_mode_stack_.back());
    this->lex_mode_stack_.pop_back();
  }

  // Return the current lexer mode.
  Lex::Mode
  lex_mode() const
  { return this->lex_mode_stack_.back(); }

  // Return the line number of the last token.
  int
  lineno() const
  { return this->lineno_; }

  // Return the character position in the line of the last token.
  int
  charpos() const
  { return this->charpos_; }

  // Return the list of input files, creating it if necessary.  This
  // is a space leak--we never free the INPUTS_ pointer.
  Input_arguments*
  inputs()
  {
    if (this->inputs_ == NULL)
      this->inputs_ = new Input_arguments();
    return this->inputs_;
  }

  // Return whether we saw any input files.
  bool
  saw_inputs() const
  { return this->inputs_ != NULL && !this->inputs_->empty(); }

  // Return the current language being processed in a version script
  // (eg, "C++").  The empty string represents unmangled C names.
  const std::string&
  get_current_language() const
  { return this->language_stack_.back(); }

  // Push a language onto the stack when entering an extern block.
  void push_language(const std::string& lang)
  { this->language_stack_.push_back(lang); }

  // Pop a language off of the stack when exiting an extern block.
  void pop_language()
  {
    gold_assert(!this->language_stack_.empty());
    this->language_stack_.pop_back();
  }

 private:
  // The name of the file we are reading.
  const char* filename_;
  // The position dependent options.
  Position_dependent_options posdep_options_;
  // Whether we are currently in a --start-group/--end-group.
  bool in_group_;
  // Whether the script was found in a sysrooted directory.
  bool is_in_sysroot_;
  // May be NULL if the user chooses not to pass one in.
  Command_line* command_line_;
  // Options which may be set from any linker script.
  Script_options* script_options_;
  // Information parsed from a version script.
  Version_script_info* version_script_info_;
  // The lexer.
  Lex* lex_;
  // The line number of the last token returned by next_token.
  int lineno_;
  // The column number of the last token returned by next_token.
  int charpos_;
  // A stack of lexer modes.
  std::vector<Lex::Mode> lex_mode_stack_;
  // A stack of which extern/language block we're inside. Can be C++,
  // java, or empty for C.
  std::vector<std::string> language_stack_;
  // New input files found to add to the link.
  Input_arguments* inputs_;
};

// FILE was found as an argument on the command line.  Try to read it
// as a script.  Return true if the file was handled.

bool
read_input_script(Workqueue* workqueue, const General_options& options,
		  Symbol_table* symtab, Layout* layout,
		  Dirsearch* dirsearch, Input_objects* input_objects,
		  Mapfile* mapfile, Input_group* input_group,
		  const Input_argument* input_argument,
		  Input_file* input_file, Task_token* next_blocker,
		  bool* used_next_blocker)
{
  *used_next_blocker = false;

  std::string input_string;
  Lex::read_file(input_file, &input_string);

  Lex lex(input_string.c_str(), input_string.length(), PARSING_LINKER_SCRIPT);

  Parser_closure closure(input_file->filename().c_str(),
			 input_argument->file().options(),
			 input_group != NULL,
			 input_file->is_in_sysroot(),
                         NULL,
			 layout->script_options(),
			 &lex);

  if (yyparse(&closure) != 0)
    return false;

  if (!closure.saw_inputs())
    return true;

  Task_token* this_blocker = NULL;
  for (Input_arguments::const_iterator p = closure.inputs()->begin();
       p != closure.inputs()->end();
       ++p)
    {
      Task_token* nb;
      if (p + 1 == closure.inputs()->end())
	nb = next_blocker;
      else
	{
	  nb = new Task_token(true);
	  nb->add_blocker();
	}
      workqueue->queue_soon(new Read_symbols(options, input_objects, symtab,
					     layout, dirsearch, mapfile, &*p,
					     input_group, this_blocker, nb));
      this_blocker = nb;
    }

  *used_next_blocker = true;

  return true;
}

// Helper function for read_version_script() and
// read_commandline_script().  Processes the given file in the mode
// indicated by first_token and lex_mode.

static bool
read_script_file(const char* filename, Command_line* cmdline,
                 int first_token, Lex::Mode lex_mode)
{
  // TODO: if filename is a relative filename, search for it manually
  // using "." + cmdline->options()->search_path() -- not dirsearch.
  Dirsearch dirsearch;

  // The file locking code wants to record a Task, but we haven't
  // started the workqueue yet.  This is only for debugging purposes,
  // so we invent a fake value.
  const Task* task = reinterpret_cast<const Task*>(-1);

  // We don't want this file to be opened in binary mode.
  Position_dependent_options posdep = cmdline->position_dependent_options();
  if (posdep.format_enum() == General_options::OBJECT_FORMAT_BINARY)
    posdep.set_format_enum(General_options::OBJECT_FORMAT_ELF);
  Input_file_argument input_argument(filename, false, "", false, posdep);
  Input_file input_file(&input_argument);
  if (!input_file.open(cmdline->options(), dirsearch, task))
    return false;

  std::string input_string;
  Lex::read_file(&input_file, &input_string);

  Lex lex(input_string.c_str(), input_string.length(), first_token);
  lex.set_mode(lex_mode);

  Parser_closure closure(filename,
			 cmdline->position_dependent_options(),
			 false,
			 input_file.is_in_sysroot(),
                         cmdline,
			 &cmdline->script_options(),
			 &lex);
  if (yyparse(&closure) != 0)
    {
      input_file.file().unlock(task);
      return false;
    }

  input_file.file().unlock(task);

  gold_assert(!closure.saw_inputs());

  return true;
}

// FILENAME was found as an argument to --script (-T).
// Read it as a script, and execute its contents immediately.

bool
read_commandline_script(const char* filename, Command_line* cmdline)
{
  return read_script_file(filename, cmdline,
                          PARSING_LINKER_SCRIPT, Lex::LINKER_SCRIPT);
}

// FILE was found as an argument to --version-script.  Read it as a
// version script, and store its contents in
// cmdline->script_options()->version_script_info().

bool
read_version_script(const char* filename, Command_line* cmdline)
{
  return read_script_file(filename, cmdline,
                          PARSING_VERSION_SCRIPT, Lex::VERSION_SCRIPT);
}

// Implement the --defsym option on the command line.  Return true if
// all is well.

bool
Script_options::define_symbol(const char* definition)
{
  Lex lex(definition, strlen(definition), PARSING_DEFSYM);
  lex.set_mode(Lex::EXPRESSION);

  // Dummy value.
  Position_dependent_options posdep_options;

  Parser_closure closure("command line", posdep_options, false, false, NULL,
			 this, &lex);

  if (yyparse(&closure) != 0)
    return false;

  gold_assert(!closure.saw_inputs());

  return true;
}

// Print the script to F for debugging.

void
Script_options::print(FILE* f) const
{
  fprintf(f, "%s: Dumping linker script\n", program_name);

  if (!this->entry_.empty())
    fprintf(f, "ENTRY(%s)\n", this->entry_.c_str());

  for (Symbol_assignments::const_iterator p =
	 this->symbol_assignments_.begin();
       p != this->symbol_assignments_.end();
       ++p)
    (*p)->print(f);

  for (Assertions::const_iterator p = this->assertions_.begin();
       p != this->assertions_.end();
       ++p)
    (*p)->print(f);

  this->script_sections_.print(f);

  this->version_script_info_.print(f);
}

// Manage mapping from keywords to the codes expected by the bison
// parser.  We construct one global object for each lex mode with
// keywords.

class Keyword_to_parsecode
{
 public:
  // The structure which maps keywords to parsecodes.
  struct Keyword_parsecode
  {
    // Keyword.
    const char* keyword;
    // Corresponding parsecode.
    int parsecode;
  };

  Keyword_to_parsecode(const Keyword_parsecode* keywords,
                       int keyword_count)
      : keyword_parsecodes_(keywords), keyword_count_(keyword_count)
  { }

  // Return the parsecode corresponding KEYWORD, or 0 if it is not a
  // keyword.
  int
  keyword_to_parsecode(const char* keyword, size_t len) const;

 private:
  const Keyword_parsecode* keyword_parsecodes_;
  const int keyword_count_;
};

// Mapping from keyword string to keyword parsecode.  This array must
// be kept in sorted order.  Parsecodes are looked up using bsearch.
// This array must correspond to the list of parsecodes in yyscript.y.

static const Keyword_to_parsecode::Keyword_parsecode
script_keyword_parsecodes[] =
{
  { "ABSOLUTE", ABSOLUTE },
  { "ADDR", ADDR },
  { "ALIGN", ALIGN_K },
  { "ALIGNOF", ALIGNOF },
  { "ASSERT", ASSERT_K },
  { "AS_NEEDED", AS_NEEDED },
  { "AT", AT },
  { "BIND", BIND },
  { "BLOCK", BLOCK },
  { "BYTE", BYTE },
  { "CONSTANT", CONSTANT },
  { "CONSTRUCTORS", CONSTRUCTORS },
  { "CREATE_OBJECT_SYMBOLS", CREATE_OBJECT_SYMBOLS },
  { "DATA_SEGMENT_ALIGN", DATA_SEGMENT_ALIGN },
  { "DATA_SEGMENT_END", DATA_SEGMENT_END },
  { "DATA_SEGMENT_RELRO_END", DATA_SEGMENT_RELRO_END },
  { "DEFINED", DEFINED },
  { "ENTRY", ENTRY },
  { "EXCLUDE_FILE", EXCLUDE_FILE },
  { "EXTERN", EXTERN },
  { "FILL", FILL },
  { "FLOAT", FLOAT },
  { "FORCE_COMMON_ALLOCATION", FORCE_COMMON_ALLOCATION },
  { "GROUP", GROUP },
  { "HLL", HLL },
  { "INCLUDE", INCLUDE },
  { "INHIBIT_COMMON_ALLOCATION", INHIBIT_COMMON_ALLOCATION },
  { "INPUT", INPUT },
  { "KEEP", KEEP },
  { "LENGTH", LENGTH },
  { "LOADADDR", LOADADDR },
  { "LONG", LONG },
  { "MAP", MAP },
  { "MAX", MAX_K },
  { "MEMORY", MEMORY },
  { "MIN", MIN_K },
  { "NEXT", NEXT },
  { "NOCROSSREFS", NOCROSSREFS },
  { "NOFLOAT", NOFLOAT },
  { "ONLY_IF_RO", ONLY_IF_RO },
  { "ONLY_IF_RW", ONLY_IF_RW },
  { "OPTION", OPTION },
  { "ORIGIN", ORIGIN },
  { "OUTPUT", OUTPUT },
  { "OUTPUT_ARCH", OUTPUT_ARCH },
  { "OUTPUT_FORMAT", OUTPUT_FORMAT },
  { "OVERLAY", OVERLAY },
  { "PHDRS", PHDRS },
  { "PROVIDE", PROVIDE },
  { "PROVIDE_HIDDEN", PROVIDE_HIDDEN },
  { "QUAD", QUAD },
  { "SEARCH_DIR", SEARCH_DIR },
  { "SECTIONS", SECTIONS },
  { "SEGMENT_START", SEGMENT_START },
  { "SHORT", SHORT },
  { "SIZEOF", SIZEOF },
  { "SIZEOF_HEADERS", SIZEOF_HEADERS },
  { "SORT", SORT_BY_NAME },
  { "SORT_BY_ALIGNMENT", SORT_BY_ALIGNMENT },
  { "SORT_BY_NAME", SORT_BY_NAME },
  { "SPECIAL", SPECIAL },
  { "SQUAD", SQUAD },
  { "STARTUP", STARTUP },
  { "SUBALIGN", SUBALIGN },
  { "SYSLIB", SYSLIB },
  { "TARGET", TARGET_K },
  { "TRUNCATE", TRUNCATE },
  { "VERSION", VERSIONK },
  { "global", GLOBAL },
  { "l", LENGTH },
  { "len", LENGTH },
  { "local", LOCAL },
  { "o", ORIGIN },
  { "org", ORIGIN },
  { "sizeof_headers", SIZEOF_HEADERS },
};

static const Keyword_to_parsecode
script_keywords(&script_keyword_parsecodes[0],
                (sizeof(script_keyword_parsecodes)
                 / sizeof(script_keyword_parsecodes[0])));

static const Keyword_to_parsecode::Keyword_parsecode
version_script_keyword_parsecodes[] =
{
  { "extern", EXTERN },
  { "global", GLOBAL },
  { "local", LOCAL },
};

static const Keyword_to_parsecode
version_script_keywords(&version_script_keyword_parsecodes[0],
                        (sizeof(version_script_keyword_parsecodes)
                         / sizeof(version_script_keyword_parsecodes[0])));

// Comparison function passed to bsearch.

extern "C"
{

struct Ktt_key
{
  const char* str;
  size_t len;
};

static int
ktt_compare(const void* keyv, const void* kttv)
{
  const Ktt_key* key = static_cast<const Ktt_key*>(keyv);
  const Keyword_to_parsecode::Keyword_parsecode* ktt =
    static_cast<const Keyword_to_parsecode::Keyword_parsecode*>(kttv);
  int i = strncmp(key->str, ktt->keyword, key->len);
  if (i != 0)
    return i;
  if (ktt->keyword[key->len] != '\0')
    return -1;
  return 0;
}

} // End extern "C".

int
Keyword_to_parsecode::keyword_to_parsecode(const char* keyword,
                                           size_t len) const
{
  Ktt_key key;
  key.str = keyword;
  key.len = len;
  void* kttv = bsearch(&key,
                       this->keyword_parsecodes_,
                       this->keyword_count_,
                       sizeof(this->keyword_parsecodes_[0]),
                       ktt_compare);
  if (kttv == NULL)
    return 0;
  Keyword_parsecode* ktt = static_cast<Keyword_parsecode*>(kttv);
  return ktt->parsecode;
}

// The following structs are used within the VersionInfo class as well
// as in the bison helper functions.  They store the information
// parsed from the version script.

// A single version expression.
// For example, pattern="std::map*" and language="C++".
// pattern and language should be from the stringpool
struct Version_expression {
  Version_expression(const std::string& pattern,
                     const std::string& language,
                     bool exact_match)
      : pattern(pattern), language(language), exact_match(exact_match) {}

  std::string pattern;
  std::string language;
  // If false, we use glob() to match pattern.  If true, we use strcmp().
  bool exact_match;
};


// A list of expressions.
struct Version_expression_list {
  std::vector<struct Version_expression> expressions;
};


// A list of which versions upon which another version depends.
// Strings should be from the Stringpool.
struct Version_dependency_list {
  std::vector<std::string> dependencies;
};


// The total definition of a version.  It includes the tag for the
// version, its global and local expressions, and any dependencies.
struct Version_tree {
  Version_tree()
      : tag(), global(NULL), local(NULL), dependencies(NULL) {}

  std::string tag;
  const struct Version_expression_list* global;
  const struct Version_expression_list* local;
  const struct Version_dependency_list* dependencies;
};

Version_script_info::~Version_script_info()
{
  this->clear();
}

void
Version_script_info::clear()
{
  for (size_t k = 0; k < dependency_lists_.size(); ++k)
    delete dependency_lists_[k];
  this->dependency_lists_.clear();
  for (size_t k = 0; k < version_trees_.size(); ++k)
    delete version_trees_[k];
  this->version_trees_.clear();
  for (size_t k = 0; k < expression_lists_.size(); ++k)
    delete expression_lists_[k];
  this->expression_lists_.clear();
}

std::vector<std::string>
Version_script_info::get_versions() const
{
  std::vector<std::string> ret;
  for (size_t j = 0; j < version_trees_.size(); ++j)
    if (!this->version_trees_[j]->tag.empty())
      ret.push_back(this->version_trees_[j]->tag);
  return ret;
}

std::vector<std::string>
Version_script_info::get_dependencies(const char* version) const
{
  std::vector<std::string> ret;
  for (size_t j = 0; j < version_trees_.size(); ++j)
    if (version_trees_[j]->tag == version)
      {
        const struct Version_dependency_list* deps =
          version_trees_[j]->dependencies;
        if (deps != NULL)
          for (size_t k = 0; k < deps->dependencies.size(); ++k)
            ret.push_back(deps->dependencies[k]);
        return ret;
      }
  return ret;
}

// Look up SYMBOL_NAME in the list of versions.  If CHECK_GLOBAL is
// true look at the globally visible symbols, otherwise look at the
// symbols listed as "local:".  Return true if the symbol is found,
// false otherwise.  If the symbol is found, then if PVERSION is not
// NULL, set *PVERSION to the version.

bool
Version_script_info::get_symbol_version_helper(const char* symbol_name,
                                               bool check_global,
					       std::string* pversion) const
{
  for (size_t j = 0; j < version_trees_.size(); ++j)
    {
      // Is it a global symbol for this version?
      const Version_expression_list* explist =
          check_global ? version_trees_[j]->global : version_trees_[j]->local;
      if (explist != NULL)
        for (size_t k = 0; k < explist->expressions.size(); ++k)
          {
            const char* name_to_match = symbol_name;
            const struct Version_expression& exp = explist->expressions[k];
            char* demangled_name = NULL;
            if (exp.language == "C++")
              {
                demangled_name = cplus_demangle(symbol_name,
                                                DMGL_ANSI | DMGL_PARAMS);
                // This isn't a C++ symbol.
                if (demangled_name == NULL)
                  continue;
                name_to_match = demangled_name;
              }
            else if (exp.language == "Java")
              {
                demangled_name = cplus_demangle(symbol_name,
                                                (DMGL_ANSI | DMGL_PARAMS
						 | DMGL_JAVA));
                // This isn't a Java symbol.
                if (demangled_name == NULL)
                  continue;
                name_to_match = demangled_name;
              }
            bool matched;
            if (exp.exact_match)
              matched = strcmp(exp.pattern.c_str(), name_to_match) == 0;
            else
              matched = fnmatch(exp.pattern.c_str(), name_to_match,
                                FNM_NOESCAPE) == 0;
            if (demangled_name != NULL)
              free(demangled_name);
            if (matched)
	      {
		if (pversion != NULL)
		  *pversion = this->version_trees_[j]->tag;
		return true;
	      }
          }
    }
  return false;
}

struct Version_dependency_list*
Version_script_info::allocate_dependency_list()
{
  dependency_lists_.push_back(new Version_dependency_list);
  return dependency_lists_.back();
}

struct Version_expression_list*
Version_script_info::allocate_expression_list()
{
  expression_lists_.push_back(new Version_expression_list);
  return expression_lists_.back();
}

struct Version_tree*
Version_script_info::allocate_version_tree()
{
  version_trees_.push_back(new Version_tree);
  return version_trees_.back();
}

// Print for debugging.

void
Version_script_info::print(FILE* f) const
{
  if (this->empty())
    return;

  fprintf(f, "VERSION {");

  for (size_t i = 0; i < this->version_trees_.size(); ++i)
    {
      const Version_tree* vt = this->version_trees_[i];

      if (vt->tag.empty())
	fprintf(f, "  {\n");
      else
	fprintf(f, "  %s {\n", vt->tag.c_str());

      if (vt->global != NULL)
	{
	  fprintf(f, "    global :\n");
	  this->print_expression_list(f, vt->global);
	}

      if (vt->local != NULL)
	{
	  fprintf(f, "    local :\n");
	  this->print_expression_list(f, vt->local);
	}

      fprintf(f, "  }");
      if (vt->dependencies != NULL)
	{
	  const Version_dependency_list* deps = vt->dependencies;
	  for (size_t j = 0; j < deps->dependencies.size(); ++j)
	    {
	      if (j < deps->dependencies.size() - 1)
		fprintf(f, "\n");
	      fprintf(f, "    %s", deps->dependencies[j].c_str());
	    }
	}
      fprintf(f, ";\n");
    }

  fprintf(f, "}\n");
}

void
Version_script_info::print_expression_list(
    FILE* f,
    const Version_expression_list* vel) const
{
  std::string current_language;
  for (size_t i = 0; i < vel->expressions.size(); ++i)
    {
      const Version_expression& ve(vel->expressions[i]);

      if (ve.language != current_language)
	{
	  if (!current_language.empty())
	    fprintf(f, "      }\n");
	  fprintf(f, "      extern \"%s\" {\n", ve.language.c_str());
	  current_language = ve.language;
	}

      fprintf(f, "      ");
      if (!current_language.empty())
	fprintf(f, "  ");

      if (ve.exact_match)
	fprintf(f, "\"");
      fprintf(f, "%s", ve.pattern.c_str());
      if (ve.exact_match)
	fprintf(f, "\"");

      fprintf(f, "\n");
    }

  if (!current_language.empty())
    fprintf(f, "      }\n");
}

} // End namespace gold.

// The remaining functions are extern "C", so it's clearer to not put
// them in namespace gold.

using namespace gold;

// This function is called by the bison parser to return the next
// token.

extern "C" int
yylex(YYSTYPE* lvalp, void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  const Token* token = closure->next_token();
  switch (token->classification())
    {
    default:
      gold_unreachable();

    case Token::TOKEN_INVALID:
      yyerror(closurev, "invalid character");
      return 0;

    case Token::TOKEN_EOF:
      return 0;

    case Token::TOKEN_STRING:
      {
	// This is either a keyword or a STRING.
	size_t len;
	const char* str = token->string_value(&len);
	int parsecode = 0;
        switch (closure->lex_mode())
          {
          case Lex::LINKER_SCRIPT:
            parsecode = script_keywords.keyword_to_parsecode(str, len);
            break;
          case Lex::VERSION_SCRIPT:
            parsecode = version_script_keywords.keyword_to_parsecode(str, len);
            break;
          default:
            break;
          }
	if (parsecode != 0)
	  return parsecode;
	lvalp->string.value = str;
	lvalp->string.length = len;
	return STRING;
      }

    case Token::TOKEN_QUOTED_STRING:
      lvalp->string.value = token->string_value(&lvalp->string.length);
      return QUOTED_STRING;

    case Token::TOKEN_OPERATOR:
      return token->operator_value();

    case Token::TOKEN_INTEGER:
      lvalp->integer = token->integer_value();
      return INTEGER;
    }
}

// This function is called by the bison parser to report an error.

extern "C" void
yyerror(void* closurev, const char* message)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  gold_error(_("%s:%d:%d: %s"), closure->filename(), closure->lineno(),
	     closure->charpos(), message);
}

// Called by the bison parser to add a file to the link.

extern "C" void
script_add_file(void* closurev, const char* name, size_t length)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);

  // If this is an absolute path, and we found the script in the
  // sysroot, then we want to prepend the sysroot to the file name.
  // For example, this is how we handle a cross link to the x86_64
  // libc.so, which refers to /lib/libc.so.6.
  std::string name_string(name, length);
  const char* extra_search_path = ".";
  std::string script_directory;
  if (IS_ABSOLUTE_PATH(name_string.c_str()))
    {
      if (closure->is_in_sysroot())
	{
	  const std::string& sysroot(parameters->options().sysroot());
	  gold_assert(!sysroot.empty());
	  name_string = sysroot + name_string;
	}
    }
  else
    {
      // In addition to checking the normal library search path, we
      // also want to check in the script-directory.
      const char *slash = strrchr(closure->filename(), '/');
      if (slash != NULL)
	{
	  script_directory.assign(closure->filename(),
				  slash - closure->filename() + 1);
	  extra_search_path = script_directory.c_str();
	}
    }

  Input_file_argument file(name_string.c_str(), false, extra_search_path,
			   false, closure->position_dependent_options());
  closure->inputs()->add_file(file);
}

// Called by the bison parser to start a group.  If we are already in
// a group, that means that this script was invoked within a
// --start-group --end-group sequence on the command line, or that
// this script was found in a GROUP of another script.  In that case,
// we simply continue the existing group, rather than starting a new
// one.  It is possible to construct a case in which this will do
// something other than what would happen if we did a recursive group,
// but it's hard to imagine why the different behaviour would be
// useful for a real program.  Avoiding recursive groups is simpler
// and more efficient.

extern "C" void
script_start_group(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (!closure->in_group())
    closure->inputs()->start_group();
}

// Called by the bison parser at the end of a group.

extern "C" void
script_end_group(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (!closure->in_group())
    closure->inputs()->end_group();
}

// Called by the bison parser to start an AS_NEEDED list.

extern "C" void
script_start_as_needed(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->position_dependent_options().set_as_needed(true);
}

// Called by the bison parser at the end of an AS_NEEDED list.

extern "C" void
script_end_as_needed(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->position_dependent_options().set_as_needed(false);
}

// Called by the bison parser to set the entry symbol.

extern "C" void
script_set_entry(void* closurev, const char* entry, size_t length)
{
  // We'll parse this exactly the same as --entry=ENTRY on the commandline
  // TODO(csilvers): FIXME -- call set_entry directly.
  std::string arg("--entry=");
  arg.append(entry, length);
  script_parse_option(closurev, arg.c_str(), arg.size());
}

// Called by the bison parser to set whether to define common symbols.

extern "C" void
script_set_common_allocation(void* closurev, int set)
{
  const char* arg = set != 0 ? "--define-common" : "--no-define-common";
  script_parse_option(closurev, arg, strlen(arg));
}

// Called by the bison parser to define a symbol.

extern "C" void
script_set_symbol(void* closurev, const char* name, size_t length,
		  Expression* value, int providei, int hiddeni)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  const bool provide = providei != 0;
  const bool hidden = hiddeni != 0;
  closure->script_options()->add_symbol_assignment(name, length, value,
						   provide, hidden);
}

// Called by the bison parser to add an assertion.

extern "C" void
script_add_assertion(void* closurev, Expression* check, const char* message,
		     size_t messagelen)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->script_options()->add_assertion(check, message, messagelen);
}

// Called by the bison parser to parse an OPTION.

extern "C" void
script_parse_option(void* closurev, const char* option, size_t length)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  // We treat the option as a single command-line option, even if
  // it has internal whitespace.
  if (closure->command_line() == NULL)
    {
      // There are some options that we could handle here--e.g.,
      // -lLIBRARY.  Should we bother?
      gold_warning(_("%s:%d:%d: ignoring command OPTION; OPTION is only valid"
		     " for scripts specified via -T/--script"),
		   closure->filename(), closure->lineno(), closure->charpos());
    }
  else
    {
      bool past_a_double_dash_option = false;
      const char* mutable_option = strndup(option, length);
      gold_assert(mutable_option != NULL);
      closure->command_line()->process_one_option(1, &mutable_option, 0,
                                                  &past_a_double_dash_option);
      // The General_options class will quite possibly store a pointer
      // into mutable_option, so we can't free it.  In cases the class
      // does not store such a pointer, this is a memory leak.  Alas. :(
    }
}

// Called by the bison parser to handle SEARCH_DIR.  This is handled
// exactly like a -L option.

extern "C" void
script_add_search_dir(void* closurev, const char* option, size_t length)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (closure->command_line() == NULL)
    gold_warning(_("%s:%d:%d: ignoring SEARCH_DIR; SEARCH_DIR is only valid"
		   " for scripts specified via -T/--script"),
		 closure->filename(), closure->lineno(), closure->charpos());
  else
    {
      std::string s = "-L" + std::string(option, length);
      script_parse_option(closurev, s.c_str(), s.size());
    }
}

/* Called by the bison parser to push the lexer into expression
   mode.  */

extern "C" void
script_push_lex_into_expression_mode(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->push_lex_mode(Lex::EXPRESSION);
}

/* Called by the bison parser to push the lexer into version
   mode.  */

extern "C" void
script_push_lex_into_version_mode(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->push_lex_mode(Lex::VERSION_SCRIPT);
}

/* Called by the bison parser to pop the lexer mode.  */

extern "C" void
script_pop_lex_mode(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->pop_lex_mode();
}

// Register an entire version node. For example:
//
// GLIBC_2.1 {
//   global: foo;
// } GLIBC_2.0;
//
// - tag is "GLIBC_2.1"
// - tree contains the information "global: foo"
// - deps contains "GLIBC_2.0"

extern "C" void
script_register_vers_node(void*,
			  const char* tag,
			  int taglen,
			  struct Version_tree *tree,
			  struct Version_dependency_list *deps)
{
  gold_assert(tree != NULL);
  tree->dependencies = deps;
  if (tag != NULL)
    tree->tag = std::string(tag, taglen);
}

// Add a dependencies to the list of existing dependencies, if any,
// and return the expanded list.

extern "C" struct Version_dependency_list *
script_add_vers_depend(void* closurev,
		       struct Version_dependency_list *all_deps,
		       const char *depend_to_add, int deplen)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (all_deps == NULL)
    all_deps = closure->version_script()->allocate_dependency_list();
  all_deps->dependencies.push_back(std::string(depend_to_add, deplen));
  return all_deps;
}

// Add a pattern expression to an existing list of expressions, if any.
// TODO: In the old linker, the last argument used to be a bool, but I
// don't know what it meant.

extern "C" struct Version_expression_list *
script_new_vers_pattern(void* closurev,
			struct Version_expression_list *expressions,
			const char *pattern, int patlen, int exact_match)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (expressions == NULL)
    expressions = closure->version_script()->allocate_expression_list();
  expressions->expressions.push_back(
      Version_expression(std::string(pattern, patlen),
                         closure->get_current_language(),
                         static_cast<bool>(exact_match)));
  return expressions;
}

// Attaches b to the end of a, and clears b.  So a = a + b and b = {}.

extern "C" struct Version_expression_list*
script_merge_expressions(struct Version_expression_list *a,
                         struct Version_expression_list *b)
{
  a->expressions.insert(a->expressions.end(),
                        b->expressions.begin(), b->expressions.end());
  // We could delete b and remove it from expressions_lists_, but
  // that's a lot of work.  This works just as well.
  b->expressions.clear();
  return a;
}

// Combine the global and local expressions into a a Version_tree.

extern "C" struct Version_tree *
script_new_vers_node(void* closurev,
		     struct Version_expression_list *global,
		     struct Version_expression_list *local)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  Version_tree* tree = closure->version_script()->allocate_version_tree();
  tree->global = global;
  tree->local = local;
  return tree;
}

// Handle a transition in language, such as at the
// start or end of 'extern "C++"'

extern "C" void
version_script_push_lang(void* closurev, const char* lang, int langlen)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->push_language(std::string(lang, langlen));
}

extern "C" void
version_script_pop_lang(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->pop_language();
}

// Called by the bison parser to start a SECTIONS clause.

extern "C" void
script_start_sections(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->script_options()->script_sections()->start_sections();
}

// Called by the bison parser to finish a SECTIONS clause.

extern "C" void
script_finish_sections(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->script_options()->script_sections()->finish_sections();
}

// Start processing entries for an output section.

extern "C" void
script_start_output_section(void* closurev, const char* name, size_t namelen,
			    const struct Parser_output_section_header* header)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->script_options()->script_sections()->start_output_section(name,
								     namelen,
								     header);
}

// Finish processing entries for an output section.

extern "C" void
script_finish_output_section(void* closurev, 
			     const struct Parser_output_section_trailer* trail)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->script_options()->script_sections()->finish_output_section(trail);
}

// Add a data item (e.g., "WORD (0)") to the current output section.

extern "C" void
script_add_data(void* closurev, int data_token, Expression* val)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  int size;
  bool is_signed = true;
  switch (data_token)
    {
    case QUAD:
      size = 8;
      is_signed = false;
      break;
    case SQUAD:
      size = 8;
      break;
    case LONG:
      size = 4;
      break;
    case SHORT:
      size = 2;
      break;
    case BYTE:
      size = 1;
      break;
    default:
      gold_unreachable();
    }
  closure->script_options()->script_sections()->add_data(size, is_signed, val);
}

// Add a clause setting the fill value to the current output section.

extern "C" void
script_add_fill(void* closurev, Expression* val)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  closure->script_options()->script_sections()->add_fill(val);
}

// Add a new input section specification to the current output
// section.

extern "C" void
script_add_input_section(void* closurev,
			 const struct Input_section_spec* spec,
			 int keepi)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  bool keep = keepi != 0;
  closure->script_options()->script_sections()->add_input_section(spec, keep);
}

// When we see DATA_SEGMENT_ALIGN we record that following output
// sections may be relro.

extern "C" void
script_data_segment_align(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (!closure->script_options()->saw_sections_clause())
    gold_error(_("%s:%d:%d: DATA_SEGMENT_ALIGN not in SECTIONS clause"),
	       closure->filename(), closure->lineno(), closure->charpos());
  else
    closure->script_options()->script_sections()->data_segment_align();
}

// When we see DATA_SEGMENT_RELRO_END we know that all output sections
// since DATA_SEGMENT_ALIGN should be relro.

extern "C" void
script_data_segment_relro_end(void* closurev)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  if (!closure->script_options()->saw_sections_clause())
    gold_error(_("%s:%d:%d: DATA_SEGMENT_ALIGN not in SECTIONS clause"),
	       closure->filename(), closure->lineno(), closure->charpos());
  else
    closure->script_options()->script_sections()->data_segment_relro_end();
}

// Create a new list of string/sort pairs.

extern "C" String_sort_list_ptr
script_new_string_sort_list(const struct Wildcard_section* string_sort)
{
  return new String_sort_list(1, *string_sort);
}

// Add an entry to a list of string/sort pairs.  The way the parser
// works permits us to simply modify the first parameter, rather than
// copy the vector.

extern "C" String_sort_list_ptr
script_string_sort_list_add(String_sort_list_ptr pv,
			    const struct Wildcard_section* string_sort)
{
  if (pv == NULL)
    return script_new_string_sort_list(string_sort);
  else
    {
      pv->push_back(*string_sort);
      return pv;
    }
}

// Create a new list of strings.

extern "C" String_list_ptr
script_new_string_list(const char* str, size_t len)
{
  return new String_list(1, std::string(str, len));
}

// Add an element to a list of strings.  The way the parser works
// permits us to simply modify the first parameter, rather than copy
// the vector.

extern "C" String_list_ptr
script_string_list_push_back(String_list_ptr pv, const char* str, size_t len)
{
  if (pv == NULL)
    return script_new_string_list(str, len);
  else
    {
      pv->push_back(std::string(str, len));
      return pv;
    }
}

// Concatenate two string lists.  Either or both may be NULL.  The way
// the parser works permits us to modify the parameters, rather than
// copy the vector.

extern "C" String_list_ptr
script_string_list_append(String_list_ptr pv1, String_list_ptr pv2)
{
  if (pv1 == NULL)
    return pv2;
  if (pv2 == NULL)
    return pv1;
  pv1->insert(pv1->end(), pv2->begin(), pv2->end());
  return pv1;
}

// Add a new program header.

extern "C" void
script_add_phdr(void* closurev, const char* name, size_t namelen,
		unsigned int type, const Phdr_info* info)
{
  Parser_closure* closure = static_cast<Parser_closure*>(closurev);
  bool includes_filehdr = info->includes_filehdr != 0;
  bool includes_phdrs = info->includes_phdrs != 0;
  bool is_flags_valid = info->is_flags_valid != 0;
  Script_sections* ss = closure->script_options()->script_sections();
  ss->add_phdr(name, namelen, type, includes_filehdr, includes_phdrs,
	       is_flags_valid, info->flags, info->load_address);
}

// Convert a program header string to a type.

#define PHDR_TYPE(NAME) { #NAME, sizeof(#NAME) - 1, elfcpp::NAME }

static struct
{
  const char* name;
  size_t namelen;
  unsigned int val;
} phdr_type_names[] =
{
  PHDR_TYPE(PT_NULL),
  PHDR_TYPE(PT_LOAD),
  PHDR_TYPE(PT_DYNAMIC),
  PHDR_TYPE(PT_INTERP),
  PHDR_TYPE(PT_NOTE),
  PHDR_TYPE(PT_SHLIB),
  PHDR_TYPE(PT_PHDR),
  PHDR_TYPE(PT_TLS),
  PHDR_TYPE(PT_GNU_EH_FRAME),
  PHDR_TYPE(PT_GNU_STACK),
  PHDR_TYPE(PT_GNU_RELRO)
};

extern "C" unsigned int
script_phdr_string_to_type(void* closurev, const char* name, size_t namelen)
{
  for (unsigned int i = 0;
       i < sizeof(phdr_type_names) / sizeof(phdr_type_names[0]);
       ++i)
    if (namelen == phdr_type_names[i].namelen
	&& strncmp(name, phdr_type_names[i].name, namelen) == 0)
      return phdr_type_names[i].val;
  yyerror(closurev, _("unknown PHDR type (try integer)"));
  return elfcpp::PT_NULL;
}
