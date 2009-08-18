// cref.cc -- cross reference for gold

// Copyright 2008 Free Software Foundation, Inc.
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

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "object.h"
#include "archive.h"
#include "cref.h"

namespace gold
{

// Class Cref_inputs.  This is used to hold the list of input files
// for cross referencing.

class Cref_inputs
{
 public:
  Cref_inputs()
    : objects_(), archives_(), current_(&this->objects_)
  { }

  // Add an input object file.
  void
  add_object(Object* object);

  // Start adding an archive.  We support nested archives for future
  // flexibility.
  void
  add_archive_start(Archive*);

  // Finish adding an archive.
  void
  add_archive_stop(Archive*);

  // Report symbol counts.
  void
  print_symbol_counts(const Symbol_table*, FILE*) const;

 private:
  // A list of input objects.
  typedef std::vector<Object*> Objects;

  // Information we record for an archive.
  struct Archive_info
  {
    // Archive name.
    std::string name;
    // List of objects included from the archive.
    Objects* objects;
    // Number of archive members.
    size_t member_count;
  };

  // A mapping from the name of an archive to the list of objects in
  // that archive.
  typedef std::map<std::string, Archive_info> Archives;

  // Report symbol counts for a list of Objects.
  void
  print_objects_symbol_counts(const Symbol_table*, FILE*, const Objects*) const;

  // Report symbol counts for an object.
  void
  print_object_symbol_counts(const Symbol_table*, FILE*, const Object*) const;

  // List of input objects.
  Objects objects_;
  // List of input archives.  This is a mapping from the archive file
  // name to the list of objects.
  Archives archives_;
  // The list to which we are currently adding objects.
  Objects* current_;
};

// Add an object.

void
Cref_inputs::add_object(Object* object)
{
  this->current_->push_back(object);
}

// Start adding an archive.

void
Cref_inputs::add_archive_start(Archive* archive)
{
  gold_assert(this->current_ == &this->objects_);
  if (this->archives_.find(archive->name()) == this->archives_.end())
    {
      Archive_info* pai = &this->archives_[archive->name()];
      pai->name = archive->filename();
      pai->objects = new Objects();
      pai->member_count = archive->count_members();
    }
  this->current_ = this->archives_[archive->name()].objects;
}

// Stop adding an archive.

void
Cref_inputs::add_archive_stop(Archive*)
{
  gold_assert(this->current_ != &this->objects_);
  this->current_ = &this->objects_;
}

// Report symbol counts for an object.

void
Cref_inputs::print_object_symbol_counts(const Symbol_table* symtab,
					FILE* f,
					const Object* object) const
{
  size_t defined, used;
  object->get_global_symbol_counts(symtab, &defined, &used);
  fprintf(f, "symbols %s %zu %zu\n", object->name().c_str(), defined, used);
}

// Report symbol counts for a list of Inputs.

void
Cref_inputs::print_objects_symbol_counts(const Symbol_table* symtab,
					 FILE* f,
					 const Objects* objects) const
{
  for (Objects::const_iterator p = objects->begin();
       p != objects->end();
       ++p)
    this->print_object_symbol_counts(symtab, f, *p);
}

// Print symbol counts.  This implements --print-symbol-counts.  This
// is intended to be easily read by a program.  This outputs a series
// of lines.  There are two different types of lines.

// The first is "symbols FILENAME DEFINED USED".  FILENAME is the name
// of an object file included in the link; for an archive, this will
// be ARCHIVEFILENAME(MEMBERNAME).  DEFINED is the number of symbols
// which the object file defines.  USED is the number of symbols which
// are used in the final output; this is the number of symbols which
// appear in the final output table as having been defined by this
// object.  These numbers will be different when weak symbols are
// used, and they will be different for dynamic objects.

// The second is "archives FILENAME MEMBERS USED".  FILENAME is the
// name of an archive file included in the link.  MEMBERS is the
// number of members of the archive.  USED is the number of archive
// members included in the link.

void
Cref_inputs::print_symbol_counts(const Symbol_table* symtab, FILE* f) const
{
  this->print_objects_symbol_counts(symtab, f, &this->objects_);
  for (Archives::const_iterator p = this->archives_.begin();
       p != this->archives_.end();
       ++p)
    {
      fprintf(f, "archive %s %zu %zu\n", p->second.name.c_str(),
	     p->second.member_count, p->second.objects->size());
      this->print_objects_symbol_counts(symtab, f, p->second.objects);
    }
}

// Class Cref.

// Make sure the Cref_inputs object has been created.

void
Cref::need_inputs()
{
  if (this->inputs_ == NULL)
    this->inputs_ = new Cref_inputs();
}

// Add an input object file.

void
Cref::add_object(Object* object)
{
  this->need_inputs();
  this->inputs_->add_object(object);
}

// Start adding an archive.

void
Cref::add_archive_start(Archive* archive)
{
  this->need_inputs();
  this->inputs_->add_archive_start(archive);
}

// Stop adding an archive.

void
Cref::add_archive_stop(Archive* archive)
{
  this->inputs_->add_archive_stop(archive);
}

// Print symbol counts.

void
Cref::print_symbol_counts(const Symbol_table* symtab) const
{
  if (parameters->options().user_set_print_symbol_counts()
      && this->inputs_ != NULL)
    {
      FILE* f;
      if (strcmp(parameters->options().print_symbol_counts(), "-") == 0)
	f = stdout;
      else
	{
	  f = fopen(parameters->options().print_symbol_counts(), "w");
	  if (f == NULL)
	    gold_error(_("cannot open symbol count file %s: %s"),
		       parameters->options().print_symbol_counts(),
		       strerror(errno));
	}
      if (f != NULL)
	this->inputs_->print_symbol_counts(symtab, f);
    }
}

} // End namespace gold.
