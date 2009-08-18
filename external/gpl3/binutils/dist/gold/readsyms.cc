// readsyms.cc -- read input file symbols for gold

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

#include <cstring>

#include "elfcpp.h"
#include "options.h"
#include "dirsearch.h"
#include "symtab.h"
#include "object.h"
#include "archive.h"
#include "script.h"
#include "readsyms.h"

namespace gold
{

// If we fail to open the object, then we won't create an Add_symbols
// task.  However, we still need to unblock the token, or else the
// link won't proceed to generate more error messages.  We can only
// unblock tokens when the workqueue lock is held, so we need a dummy
// task to do that.  The dummy task has to maintain the right sequence
// of blocks, so we need both this_blocker and next_blocker.

class Unblock_token : public Task
{
 public:
  Unblock_token(Task_token* this_blocker, Task_token* next_blocker)
    : this_blocker_(this_blocker), next_blocker_(next_blocker)
  { }

  ~Unblock_token()
  {
    if (this->this_blocker_ != NULL)
      delete this->this_blocker_;
  }

  Task_token*
  is_runnable()
  {
    if (this->this_blocker_ != NULL && this->this_blocker_->is_blocked())
      return this->this_blocker_;
    return NULL;
  }

  void
  locks(Task_locker* tl)
  { tl->add(this, this->next_blocker_); }

  void
  run(Workqueue*)
  { }

  std::string
  get_name() const
  { return "Unblock_token"; }

 private:
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// Class read_symbols.

Read_symbols::~Read_symbols()
{
  // The this_blocker_ and next_blocker_ pointers are passed on to the
  // Add_symbols task.
}

// Return whether a Read_symbols task is runnable.  We can read an
// ordinary input file immediately.  For an archive specified using
// -l, we have to wait until the search path is complete.

Task_token*
Read_symbols::is_runnable()
{
  if (this->input_argument_->is_file()
      && this->input_argument_->file().may_need_search()
      && this->dirpath_->token()->is_blocked())
    return this->dirpath_->token();

  return NULL;
}

// Return a Task_locker for a Read_symbols task.  We don't need any
// locks here.

void
Read_symbols::locks(Task_locker*)
{
}

// Run a Read_symbols task.

void
Read_symbols::run(Workqueue* workqueue)
{
  // If we didn't queue a new task, then we need to explicitly unblock
  // the token.
  if (!this->do_read_symbols(workqueue))
    workqueue->queue_soon(new Unblock_token(this->this_blocker_,
					    this->next_blocker_));
}

// Open the file and read the symbols.  Return true if a new task was
// queued, false if that could not happen due to some error.

bool
Read_symbols::do_read_symbols(Workqueue* workqueue)
{
  if (this->input_argument_->is_group())
    {
      gold_assert(this->input_group_ == NULL);
      this->do_group(workqueue);
      return true;
    }

  Input_file* input_file = new Input_file(&this->input_argument_->file());
  if (!input_file->open(this->options_, *this->dirpath_, this))
    return false;

  // Read enough of the file to pick up the entire ELF header.

  off_t filesize = input_file->file().filesize();

  if (filesize == 0)
    {
      gold_error(_("%s: file is empty"),
		 input_file->file().filename().c_str());
      return false;
    }

  int read_size = elfcpp::Elf_sizes<64>::ehdr_size;
  if (filesize < read_size)
    read_size = filesize;

  const unsigned char* ehdr = input_file->file().get_view(0, 0, read_size,
							  true, false);

  if (read_size >= 4)
    {
      static unsigned char elfmagic[4] =
	{
	  elfcpp::ELFMAG0, elfcpp::ELFMAG1,
	  elfcpp::ELFMAG2, elfcpp::ELFMAG3
	};
      if (memcmp(ehdr, elfmagic, 4) == 0)
	{
	  // This is an ELF object.

	  Object* obj = make_elf_object(input_file->filename(),
					input_file, 0, ehdr, read_size);
	  if (obj == NULL)
	    return false;

	  Read_symbols_data* sd = new Read_symbols_data;
	  obj->read_symbols(sd);

	  // Opening the file locked it, so now we need to unlock it.
	  // We need to unlock it before queuing the Add_symbols task,
	  // because the workqueue doesn't know about our lock on the
	  // file.  If we queue the Add_symbols task first, it will be
	  // stuck on the end of the file lock, but since the
	  // workqueue doesn't know about that lock, it will never
	  // release the Add_symbols task.

	  input_file->file().unlock(this);

	  // We use queue_next because everything is cached for this
	  // task to run right away if possible.

	  workqueue->queue_next(new Add_symbols(this->input_objects_,
						this->symtab_, this->layout_,
						obj, sd,
						this->this_blocker_,
						this->next_blocker_));

	  return true;
	}
    }

  if (read_size >= Archive::sarmag)
    {
      bool is_thin_archive
          = memcmp(ehdr, Archive::armagt, Archive::sarmag) == 0;
      if (is_thin_archive 
          || memcmp(ehdr, Archive::armag, Archive::sarmag) == 0)
	{
	  // This is an archive.
	  Archive* arch = new Archive(this->input_argument_->file().name(),
				      input_file, is_thin_archive,
				      this->dirpath_, this);
	  arch->setup(this->input_objects_);
	  
	  // Unlock the archive so it can be used in the next task.
	  arch->unlock(this);

	  workqueue->queue_next(new Add_archive_symbols(this->symtab_,
							this->layout_,
							this->input_objects_,
							this->mapfile_,
							arch,
							this->input_group_,
							this->this_blocker_,
							this->next_blocker_));
	  return true;
	}
    }

  // Queue up a task to try to parse this file as a script.  We use a
  // separate task so that the script will be read in order with other
  // objects named on the command line.  Also so that we don't try to
  // read multiple scripts simultaneously, which could lead to
  // unpredictable changes to the General_options structure.

  workqueue->queue_soon(new Read_script(this->options_,
					this->symtab_,
					this->layout_,
					this->dirpath_,
					this->input_objects_,
					this->mapfile_,
					this->input_group_,
					this->input_argument_,
					input_file,
					this->this_blocker_,
					this->next_blocker_));
  return true;
}

// Handle a group.  We need to walk through the arguments over and
// over until we don't see any new undefined symbols.  We do this by
// setting off Read_symbols Tasks as usual, but recording the archive
// entries instead of deleting them.  We also start a Finish_group
// Task which runs after we've read all the symbols.  In that task we
// process the archives in a loop until we are done.

void
Read_symbols::do_group(Workqueue* workqueue)
{
  Input_group* input_group = new Input_group();

  const Input_file_group* group = this->input_argument_->group();
  Task_token* this_blocker = this->this_blocker_;

  for (Input_file_group::const_iterator p = group->begin();
       p != group->end();
       ++p)
    {
      const Input_argument* arg = &*p;
      gold_assert(arg->is_file());

      Task_token* next_blocker = new Task_token(true);
      next_blocker->add_blocker();
      workqueue->queue_soon(new Read_symbols(this->options_,
					     this->input_objects_,
					     this->symtab_, this->layout_,
					     this->dirpath_, this->mapfile_,
					     arg, input_group,
					     this_blocker, next_blocker));
      this_blocker = next_blocker;
    }

  const int saw_undefined = this->symtab_->saw_undefined();
  workqueue->queue_soon(new Finish_group(this->input_objects_,
					 this->symtab_,
					 this->layout_,
					 this->mapfile_,
					 input_group,
					 saw_undefined,
					 this_blocker,
					 this->next_blocker_));
}

// Return a debugging name for a Read_symbols task.

std::string
Read_symbols::get_name() const
{
  if (!this->input_argument_->is_group())
    {
      std::string ret("Read_symbols ");
      if (this->input_argument_->file().is_lib())
	ret += "-l";
      ret += this->input_argument_->file().name();
      return ret;
    }

  std::string ret("Read_symbols group (");
  bool add_space = false;
  const Input_file_group* group = this->input_argument_->group();
  for (Input_file_group::const_iterator p = group->begin();
       p != group->end();
       ++p)
    {
      if (add_space)
	ret += ' ';
      ret += p->file().name();
      add_space = true;
    }
  return ret + ')';
}

// Class Add_symbols.

Add_symbols::~Add_symbols()
{
  if (this->this_blocker_ != NULL)
    delete this->this_blocker_;
  // next_blocker_ is deleted by the task associated with the next
  // input file.
}

// We are blocked by this_blocker_.  We block next_blocker_.  We also
// lock the file.

Task_token*
Add_symbols::is_runnable()
{
  if (this->this_blocker_ != NULL && this->this_blocker_->is_blocked())
    return this->this_blocker_;
  if (this->object_->is_locked())
    return this->object_->token();
  return NULL;
}

void
Add_symbols::locks(Task_locker* tl)
{
  tl->add(this, this->next_blocker_);
  tl->add(this, this->object_->token());
}

// Add the symbols in the object to the symbol table.

void
Add_symbols::run(Workqueue*)
{
  if (!this->input_objects_->add_object(this->object_))
    {
      // FIXME: We need to close the descriptor here.
      delete this->object_;
    }
  else
    {
      this->object_->layout(this->symtab_, this->layout_, this->sd_);
      this->object_->add_symbols(this->symtab_, this->sd_);
      this->object_->release();
    }
  delete this->sd_;
  this->sd_ = NULL;
}

// Class Finish_group.

Finish_group::~Finish_group()
{
  if (this->this_blocker_ != NULL)
    delete this->this_blocker_;
  // next_blocker_ is deleted by the task associated with the next
  // input file following the group.
}

// We need to wait for THIS_BLOCKER_ and unblock NEXT_BLOCKER_.

Task_token*
Finish_group::is_runnable()
{
  if (this->this_blocker_ != NULL && this->this_blocker_->is_blocked())
    return this->this_blocker_;
  return NULL;
}

void
Finish_group::locks(Task_locker* tl)
{
  tl->add(this, this->next_blocker_);
}

// Loop over the archives until there are no new undefined symbols.

void
Finish_group::run(Workqueue*)
{
  int saw_undefined = this->saw_undefined_;
  while (saw_undefined != this->symtab_->saw_undefined())
    {
      saw_undefined = this->symtab_->saw_undefined();

      for (Input_group::const_iterator p = this->input_group_->begin();
	   p != this->input_group_->end();
	   ++p)
	{
	  Task_lock_obj<Archive> tl(this, *p);

	  (*p)->add_symbols(this->symtab_, this->layout_,
			    this->input_objects_, this->mapfile_);
	}
    }

  // Delete all the archives now that we no longer need them.
  for (Input_group::const_iterator p = this->input_group_->begin();
       p != this->input_group_->end();
       ++p)
    delete *p;
  delete this->input_group_;
}

// Class Read_script

Read_script::~Read_script()
{
  if (this->this_blocker_ != NULL)
    delete this->this_blocker_;
  // next_blocker_ is deleted by the task associated with the next
  // input file.
}

// We are blocked by this_blocker_.

Task_token*
Read_script::is_runnable()
{
  if (this->this_blocker_ != NULL && this->this_blocker_->is_blocked())
    return this->this_blocker_;
  return NULL;
}

// We don't unlock next_blocker_ here.  If the script names any input
// files, then the last file will be responsible for unlocking it.

void
Read_script::locks(Task_locker*)
{
}

// Read the script, if it is a script.

void
Read_script::run(Workqueue* workqueue)
{
  bool used_next_blocker;
  if (!read_input_script(workqueue, this->options_, this->symtab_,
			 this->layout_, this->dirpath_, this->input_objects_,
			 this->mapfile_, this->input_group_,
			 this->input_argument_, this->input_file_,
			 this->next_blocker_, &used_next_blocker))
    {
      // Here we have to handle any other input file types we need.
      gold_error(_("%s: not an object or archive"),
		 this->input_file_->file().filename().c_str());
    }

  if (!used_next_blocker)
    {
      // Queue up a task to unlock next_blocker.  We can't just unlock
      // it here, as we don't hold the workqueue lock.
      workqueue->queue_soon(new Unblock_token(NULL, this->next_blocker_));
    }
}

// Return a debugging name for a Read_script task.

std::string
Read_script::get_name() const
{
  std::string ret("Read_script ");
  if (this->input_argument_->file().is_lib())
    ret += "-l";
  ret += this->input_argument_->file().name();
  return ret;
}

} // End namespace gold.
