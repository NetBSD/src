/* Copyright (C) 2025 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _Symbol_h_
#define _Symbol_h_

class Function;
class Module;

class Range
{
public:
  Range (uint64_t _low, uint64_t _high)
  {
    low = _low;
    high = _high;
  }

  inline bool
  inside (uint64_t val)
  {
    return val >= low && val < high;
  };

  uint64_t low;
  uint64_t high;
};

class Symbol
{
public:
  Symbol (Vector<Symbol *> *vec = NULL);
  ~Symbol ();

  Symbol *
  cardinal ()
  {
    return alias ? alias : this;
  }

  // Find symbols in 'syms' matched by 'ranges'.
  static Vector<Symbol *> *find_symbols (Vector<Symbol *> *syms,
		    Vector<Range *> *ranges, Vector<Symbol *> *symbols = NULL);
  static Vector<Symbol *> *sort_by_name (Vector<Symbol *> *syms);

  // Find symbol in CU corresponding to pc or linker_name.
  static Symbol *get_symbol (Vector<Symbol *> *syms, uint64_t pc);
  static Symbol *get_symbol (Vector<Symbol *> *syms, char *linker_name);

  // Create and append a new function to the 'module'.
  // Copy attributes (size, name, etc) from Symbol,
  Function *createFunction(Module *module);
  void dump (const char *msg = NULL);

  Function *func;
  Sp_lang_code lang_code;
  uint64_t value;
  uint64_t save;
  int64_t size;
  uint64_t img_offset; // image offset in the ELF file
  char *name;
  Symbol *alias;
  int local_ind;
  int flags;
  bool defined;
};
#endif
