// readsyms.h -- read input file symbols for gold   -*- C++ -*-

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

#ifndef GOLD_READSYMS_H
#define GOLD_READSYMS_H

#include <vector>

#include "workqueue.h"
#include "object.h"

namespace gold
{

class Input_objects;
class Symbol_table;
class Input_group;
class Archive;

// This Task is responsible for reading the symbols from an input
// file.  This also includes reading the relocations so that we can
// check for any that require a PLT and/or a GOT.  After the data has
// been read, this queues up another task to actually add the symbols
// to the symbol table.  The tasks are separated because the file
// reading can occur in parallel but adding the symbols must be done
// in the order of the input files.

class Read_symbols : public Task
{
 public:
  // DIRPATH is the list of directories to search for libraries.
  // INPUT is the file to read.  INPUT_GROUP is not NULL if we are in
  // the middle of an input group.  THIS_BLOCKER is used to prevent
  // the associated Add_symbols task from running before the previous
  // one has completed; it will be NULL for the first task.
  // NEXT_BLOCKER is used to block the next input file from adding
  // symbols.
  Read_symbols(const General_options& options, Input_objects* input_objects,
	       Symbol_table* symtab, Layout* layout, Dirsearch* dirpath,
	       Mapfile* mapfile, const Input_argument* input_argument,
	       Input_group* input_group, Task_token* this_blocker,
	       Task_token* next_blocker)
    : options_(options), input_objects_(input_objects), symtab_(symtab),
      layout_(layout), dirpath_(dirpath), mapfile_(mapfile),
      input_argument_(input_argument), input_group_(input_group),
      this_blocker_(this_blocker), next_blocker_(next_blocker)
  { }

  ~Read_symbols();

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const;

 private:
  // Handle an archive group.
  void
  do_group(Workqueue*);

  // Open and identify the file.
  bool
  do_read_symbols(Workqueue*);

  const General_options& options_;
  Input_objects* input_objects_;
  Symbol_table* symtab_;
  Layout* layout_;
  Dirsearch* dirpath_;
  Mapfile* mapfile_;
  const Input_argument* input_argument_;
  Input_group* input_group_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// This Task handles adding the symbols to the symbol table.  These
// tasks must be run in the same order as the arguments appear on the
// command line.

class Add_symbols : public Task
{
 public:
  // THIS_BLOCKER is used to prevent this task from running before the
  // one for the previous input file.  NEXT_BLOCKER is used to prevent
  // the next task from running.
  Add_symbols(Input_objects* input_objects, Symbol_table* symtab,
	      Layout* layout, Object* object,
	      Read_symbols_data* sd, Task_token* this_blocker,
	      Task_token* next_blocker)
    : input_objects_(input_objects), symtab_(symtab), layout_(layout),
      object_(object), sd_(sd), this_blocker_(this_blocker),
      next_blocker_(next_blocker)
  { }

  ~Add_symbols();

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const
  { return "Add_symbols " + this->object_->name(); }

private:
  Input_objects* input_objects_;
  Symbol_table* symtab_;
  Layout* layout_;
  Object* object_;
  Read_symbols_data* sd_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// This class is used to track the archives in a group.

class Input_group
{
 public:
  typedef std::vector<Archive*> Archives;
  typedef Archives::const_iterator const_iterator;

  Input_group()
    : archives_()
  { }

  // Add an archive to the group.
  void
  add_archive(Archive* arch)
  { this->archives_.push_back(arch); }

  // Loop over the archives in the group.

  const_iterator
  begin() const
  { return this->archives_.begin(); }

  const_iterator
  end() const
  { return this->archives_.end(); }

 private:
  Archives archives_;
};

// This class is used to finish up handling a group.  It is just a
// closure.

class Finish_group : public Task
{
 public:
  Finish_group(Input_objects* input_objects, Symbol_table* symtab,
	       Layout* layout, Mapfile* mapfile, Input_group* input_group,
	       int saw_undefined, Task_token* this_blocker,
	       Task_token* next_blocker)
    : input_objects_(input_objects), symtab_(symtab),
      layout_(layout), mapfile_(mapfile), input_group_(input_group),
      saw_undefined_(saw_undefined), this_blocker_(this_blocker),
      next_blocker_(next_blocker)
  { }

  ~Finish_group();

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const
  { return "Finish_group"; }

 private:
  Input_objects* input_objects_;
  Symbol_table* symtab_;
  Layout* layout_;
  Mapfile* mapfile_;
  Input_group* input_group_;
  int saw_undefined_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// This class is used to read a file which was not recognized as an
// object or archive.  It tries to read it as a linker script, using
// the tokens to serialize with the calls to Add_symbols.

class Read_script : public Task
{
 public:
  Read_script(const General_options& options, Symbol_table* symtab,
	      Layout* layout, Dirsearch* dirpath, Input_objects* input_objects,
	      Mapfile* mapfile, Input_group* input_group,
	      const Input_argument* input_argument,
	      Input_file* input_file, Task_token* this_blocker,
	      Task_token* next_blocker)
    : options_(options), symtab_(symtab), layout_(layout), dirpath_(dirpath),
      input_objects_(input_objects), mapfile_(mapfile),
      input_group_(input_group), input_argument_(input_argument),
      input_file_(input_file), this_blocker_(this_blocker),
      next_blocker_(next_blocker)
  { }

  ~Read_script();

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const;

 private:
  const General_options& options_;
  Symbol_table* symtab_;
  Layout* layout_;
  Dirsearch* dirpath_;
  Input_objects* input_objects_;
  Mapfile* mapfile_;
  Input_group* input_group_;
  const Input_argument* input_argument_;
  Input_file* input_file_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

} // end namespace gold

#endif // !defined(GOLD_READSYMS_H)
