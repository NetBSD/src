// fileread.cc -- read files for gold

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
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include "filenames.h"

#include "debug.h"
#include "parameters.h"
#include "options.h"
#include "dirsearch.h"
#include "target.h"
#include "binary.h"
#include "descriptors.h"
#include "fileread.h"

namespace gold
{

// Class File_read::View.

File_read::View::~View()
{
  gold_assert(!this->is_locked());
  if (!this->mapped_)
    delete[] this->data_;
  else
    {
      if (::munmap(const_cast<unsigned char*>(this->data_), this->size_) != 0)
        gold_warning(_("munmap failed: %s"), strerror(errno));

      File_read::current_mapped_bytes -= this->size_;
    }
}

void
File_read::View::lock()
{
  ++this->lock_count_;
}

void
File_read::View::unlock()
{
  gold_assert(this->lock_count_ > 0);
  --this->lock_count_;
}

bool
File_read::View::is_locked()
{
  return this->lock_count_ > 0;
}

// Class File_read.

// The File_read static variables.
unsigned long long File_read::total_mapped_bytes;
unsigned long long File_read::current_mapped_bytes;
unsigned long long File_read::maximum_mapped_bytes;

File_read::~File_read()
{
  gold_assert(this->token_.is_writable());
  if (this->is_descriptor_opened_)
    {
      release_descriptor(this->descriptor_, true);
      this->descriptor_ = -1;
      this->is_descriptor_opened_ = false;
    }
  this->name_.clear();
  this->clear_views(true);
}

// Open the file.

bool
File_read::open(const Task* task, const std::string& name)
{
  gold_assert(this->token_.is_writable()
	      && this->descriptor_ < 0
	      && !this->is_descriptor_opened_
	      && this->name_.empty());
  this->name_ = name;

  this->descriptor_ = open_descriptor(-1, this->name_.c_str(),
				      O_RDONLY);

  if (this->descriptor_ >= 0)
    {
      this->is_descriptor_opened_ = true;
      struct stat s;
      if (::fstat(this->descriptor_, &s) < 0)
	gold_error(_("%s: fstat failed: %s"),
		   this->name_.c_str(), strerror(errno));
      this->size_ = s.st_size;
      gold_debug(DEBUG_FILES, "Attempt to open %s succeeded",
                 this->name_.c_str());

      this->token_.add_writer(task);
    }

  return this->descriptor_ >= 0;
}

// Open the file with the contents in memory.

bool
File_read::open(const Task* task, const std::string& name,
		const unsigned char* contents, off_t size)
{
  gold_assert(this->token_.is_writable()
	      && this->descriptor_ < 0
	      && !this->is_descriptor_opened_
	      && this->name_.empty());
  this->name_ = name;
  this->contents_ = contents;
  this->size_ = size;
  this->token_.add_writer(task);
  return true;
}

// Reopen a descriptor if necessary.

void
File_read::reopen_descriptor()
{
  if (!this->is_descriptor_opened_)
    {
      this->descriptor_ = open_descriptor(this->descriptor_,
					  this->name_.c_str(),
					  O_RDONLY);
      if (this->descriptor_ < 0)
	gold_fatal(_("could not reopen file %s"), this->name_.c_str());
      this->is_descriptor_opened_ = true;
    }
}

// Release the file.  This is called when we are done with the file in
// a Task.

void
File_read::release()
{
  gold_assert(this->is_locked());

  File_read::total_mapped_bytes += this->mapped_bytes_;
  File_read::current_mapped_bytes += this->mapped_bytes_;
  this->mapped_bytes_ = 0;
  if (File_read::current_mapped_bytes > File_read::maximum_mapped_bytes)
    File_read::maximum_mapped_bytes = File_read::current_mapped_bytes;

  // Only clear views if there is only one attached object.  Otherwise
  // we waste time trying to clear cached archive views.  Similarly
  // for releasing the descriptor.
  if (this->object_count_ <= 1)
    {
      this->clear_views(false);
      if (this->is_descriptor_opened_)
	{
	  release_descriptor(this->descriptor_, false);
	  this->is_descriptor_opened_ = false;
	}
    }

  this->released_ = true;
}

// Lock the file.

void
File_read::lock(const Task* task)
{
  gold_assert(this->released_);
  this->token_.add_writer(task);
  this->released_ = false;
}

// Unlock the file.

void
File_read::unlock(const Task* task)
{
  this->release();
  this->token_.remove_writer(task);
}

// Return whether the file is locked.

bool
File_read::is_locked() const
{
  if (!this->token_.is_writable())
    return true;
  // The file is not locked, so it should have been released.
  gold_assert(this->released_);
  return false;
}

// See if we have a view which covers the file starting at START for
// SIZE bytes.  Return a pointer to the View if found, NULL if not.
// If BYTESHIFT is not -1U, the returned View must have the specified
// byte shift; otherwise, it may have any byte shift.  If VSHIFTED is
// not NULL, this sets *VSHIFTED to a view which would have worked if
// not for the requested BYTESHIFT.

inline File_read::View*
File_read::find_view(off_t start, section_size_type size,
		     unsigned int byteshift, File_read::View** vshifted) const
{
  if (vshifted != NULL)
    *vshifted = NULL;

  off_t page = File_read::page_offset(start);

  unsigned int bszero = 0;
  Views::const_iterator p = this->views_.upper_bound(std::make_pair(page - 1,
								    bszero));

  while (p != this->views_.end() && p->first.first <= page)
    {
      if (p->second->start() <= start
	  && (p->second->start() + static_cast<off_t>(p->second->size())
	      >= start + static_cast<off_t>(size)))
	{
	  if (byteshift == -1U || byteshift == p->second->byteshift())
	    {
	      p->second->set_accessed();
	      return p->second;
	    }

	  if (vshifted != NULL && *vshifted == NULL)
	    *vshifted = p->second;
	}

      ++p;
    }

  return NULL;
}

// Read SIZE bytes from the file starting at offset START.  Read into
// the buffer at P.

void
File_read::do_read(off_t start, section_size_type size, void* p)
{
  ssize_t bytes;
  if (this->contents_ != NULL)
    {
      bytes = this->size_ - start;
      if (static_cast<section_size_type>(bytes) >= size)
	{
	  memcpy(p, this->contents_ + start, size);
	  return;
	}
    }
  else
    {
      this->reopen_descriptor();
      bytes = ::pread(this->descriptor_, p, size, start);
      if (static_cast<section_size_type>(bytes) == size)
	return;

      if (bytes < 0)
	{
	  gold_fatal(_("%s: pread failed: %s"),
		     this->filename().c_str(), strerror(errno));
	  return;
	}
    }

  gold_fatal(_("%s: file too short: read only %lld of %lld bytes at %lld"),
	     this->filename().c_str(),
	     static_cast<long long>(bytes),
	     static_cast<long long>(size),
	     static_cast<long long>(start));
}

// Read data from the file.

void
File_read::read(off_t start, section_size_type size, void* p)
{
  const File_read::View* pv = this->find_view(start, size, -1U, NULL);
  if (pv != NULL)
    {
      memcpy(p, pv->data() + (start - pv->start() + pv->byteshift()), size);
      return;
    }

  this->do_read(start, size, p);
}

// Add a new view.  There may already be an existing view at this
// offset.  If there is, the new view will be larger, and should
// replace the old view.

void
File_read::add_view(File_read::View* v)
{
  std::pair<Views::iterator, bool> ins =
    this->views_.insert(std::make_pair(std::make_pair(v->start(),
						      v->byteshift()),
				       v));
  if (ins.second)
    return;

  // There was an existing view at this offset.  It must not be large
  // enough.  We can't delete it here, since something might be using
  // it; we put it on a list to be deleted when the file is unlocked.
  File_read::View* vold = ins.first->second;
  gold_assert(vold->size() < v->size());
  if (vold->should_cache())
    {
      v->set_cache();
      vold->clear_cache();
    }
  this->saved_views_.push_back(vold);

  ins.first->second = v;
}

// Make a new view with a specified byteshift, reading the data from
// the file.

File_read::View*
File_read::make_view(off_t start, section_size_type size,
		     unsigned int byteshift, bool cache)
{
  gold_assert(size > 0);

  off_t poff = File_read::page_offset(start);

  section_size_type psize = File_read::pages(size + (start - poff));

  if (poff + static_cast<off_t>(psize) >= this->size_)
    {
      psize = this->size_ - poff;
      gold_assert(psize >= size);
    }

  File_read::View* v;
  if (this->contents_ != NULL || byteshift != 0)
    {
      unsigned char* p = new unsigned char[psize + byteshift];
      memset(p, 0, byteshift);
      this->do_read(poff, psize, p + byteshift);
      v = new File_read::View(poff, psize, p, byteshift, cache, false);
    }
  else
    {
      this->reopen_descriptor();
      void* p = ::mmap(NULL, psize, PROT_READ, MAP_PRIVATE,
                       this->descriptor_, poff);
      if (p == MAP_FAILED)
	gold_fatal(_("%s: mmap offset %lld size %lld failed: %s"),
		   this->filename().c_str(),
		   static_cast<long long>(poff),
		   static_cast<long long>(psize),
		   strerror(errno));

      this->mapped_bytes_ += psize;

      const unsigned char* pbytes = static_cast<const unsigned char*>(p);
      v = new File_read::View(poff, psize, pbytes, 0, cache, true);
    }

  this->add_view(v);

  return v;
}

// Find a View or make a new one, shifted as required by the file
// offset OFFSET and ALIGNED.

File_read::View*
File_read::find_or_make_view(off_t offset, off_t start,
			     section_size_type size, bool aligned, bool cache)
{
  unsigned int byteshift;
  if (offset == 0)
    byteshift = 0;
  else
    {
      unsigned int target_size = (!parameters->target_valid()
				  ? 64
				  : parameters->target().get_size());
      byteshift = offset & ((target_size / 8) - 1);

      // Set BYTESHIFT to the number of dummy bytes which must be
      // inserted before the data in order for this data to be
      // aligned.
      if (byteshift != 0)
	byteshift = (target_size / 8) - byteshift;
    }

  // Try to find a View with the required BYTESHIFT.
  File_read::View* vshifted;
  File_read::View* v = this->find_view(offset + start, size,
				       aligned ? byteshift : -1U,
				       &vshifted);
  if (v != NULL)
    {
      if (cache)
	v->set_cache();
      return v;
    }

  // If VSHIFTED is not NULL, then it has the data we need, but with
  // the wrong byteshift.
  v = vshifted;
  if (v != NULL)
    {
      gold_assert(aligned);

      unsigned char* pbytes = new unsigned char[v->size() + byteshift];
      memset(pbytes, 0, byteshift);
      memcpy(pbytes + byteshift, v->data() + v->byteshift(), v->size());

      File_read::View* shifted_view = new File_read::View(v->start(), v->size(),
							  pbytes, byteshift,
							  cache, false);

      this->add_view(shifted_view);
      return shifted_view;
    }

  // Make a new view.  If we don't need an aligned view, use a
  // byteshift of 0, so that we can use mmap.
  return this->make_view(offset + start, size,
			 aligned ? byteshift : 0,
			 cache);
}

// Get a view into the file.

const unsigned char*
File_read::get_view(off_t offset, off_t start, section_size_type size,
		    bool aligned, bool cache)
{
  File_read::View* pv = this->find_or_make_view(offset, start, size,
						aligned, cache);
  return pv->data() + (offset + start - pv->start() + pv->byteshift());
}

File_view*
File_read::get_lasting_view(off_t offset, off_t start, section_size_type size,
			    bool aligned, bool cache)
{
  File_read::View* pv = this->find_or_make_view(offset, start, size,
						aligned, cache);
  pv->lock();
  return new File_view(*this, pv,
		       (pv->data()
			+ (offset + start - pv->start() + pv->byteshift())));
}

// Use readv to read COUNT entries from RM starting at START.  BASE
// must be added to all file offsets in RM.

void
File_read::do_readv(off_t base, const Read_multiple& rm, size_t start,
		    size_t count)
{
  unsigned char discard[File_read::page_size];
  iovec iov[File_read::max_readv_entries * 2];
  size_t iov_index = 0;

  off_t first_offset = rm[start].file_offset;
  off_t last_offset = first_offset;
  ssize_t want = 0;
  for (size_t i = 0; i < count; ++i)
    {
      const Read_multiple_entry& i_entry(rm[start + i]);

      if (i_entry.file_offset > last_offset)
	{
	  size_t skip = i_entry.file_offset - last_offset;
	  gold_assert(skip <= sizeof discard);

	  iov[iov_index].iov_base = discard;
	  iov[iov_index].iov_len = skip;
	  ++iov_index;

	  want += skip;
	}

      iov[iov_index].iov_base = i_entry.buffer;
      iov[iov_index].iov_len = i_entry.size;
      ++iov_index;

      want += i_entry.size;

      last_offset = i_entry.file_offset + i_entry.size;
    }

  this->reopen_descriptor();

  gold_assert(iov_index < sizeof iov / sizeof iov[0]);

  if (::lseek(this->descriptor_, base + first_offset, SEEK_SET) < 0)
    gold_fatal(_("%s: lseek failed: %s"),
	       this->filename().c_str(), strerror(errno));

  ssize_t got = ::readv(this->descriptor_, iov, iov_index);

  if (got < 0)
    gold_fatal(_("%s: readv failed: %s"),
	       this->filename().c_str(), strerror(errno));
  if (got != want)
    gold_fatal(_("%s: file too short: read only %zd of %zd bytes at %lld"),
	       this->filename().c_str(),
	       got, want, static_cast<long long>(base + first_offset));
}

// Read several pieces of data from the file.

void
File_read::read_multiple(off_t base, const Read_multiple& rm)
{
  size_t count = rm.size();
  size_t i = 0;
  while (i < count)
    {
      // Find up to MAX_READV_ENTRIES consecutive entries which are
      // less than one page apart.
      const Read_multiple_entry& i_entry(rm[i]);
      off_t i_off = i_entry.file_offset;
      off_t end_off = i_off + i_entry.size;
      size_t j;
      for (j = i + 1; j < count; ++j)
	{
	  if (j - i >= File_read::max_readv_entries)
	    break;
	  const Read_multiple_entry& j_entry(rm[j]);
	  off_t j_off = j_entry.file_offset;
	  gold_assert(j_off >= end_off);
	  off_t j_end_off = j_off + j_entry.size;
	  if (j_end_off - end_off >= File_read::page_size)
	    break;
	  end_off = j_end_off;
	}

      if (j == i + 1)
	this->read(base + i_off, i_entry.size, i_entry.buffer);
      else
	{
	  File_read::View* view = this->find_view(base + i_off,
						  end_off - i_off,
						  -1U, NULL);
	  if (view == NULL)
	    this->do_readv(base, rm, i, j - i);
	  else
	    {
	      const unsigned char* v = (view->data()
					+ (base + i_off - view->start()
					   + view->byteshift()));
	      for (size_t k = i; k < j; ++k)
		{
		  const Read_multiple_entry& k_entry(rm[k]);
		  gold_assert((convert_to_section_size_type(k_entry.file_offset
                                                           - i_off)
                               + k_entry.size)
			      <= convert_to_section_size_type(end_off
                                                              - i_off));
		  memcpy(k_entry.buffer,
			 v + (k_entry.file_offset - i_off),
			 k_entry.size);
		}
	    }
	}

      i = j;
    }
}

// Mark all views as no longer cached.

void
File_read::clear_view_cache_marks()
{
  // Just ignore this if there are multiple objects associated with
  // the file.  Otherwise we will wind up uncaching and freeing some
  // views for other objects.
  if (this->object_count_ > 1)
    return;

  for (Views::iterator p = this->views_.begin();
       p != this->views_.end();
       ++p)
    p->second->clear_cache();
  for (Saved_views::iterator p = this->saved_views_.begin();
       p != this->saved_views_.end();
       ++p)
    (*p)->clear_cache();
}

// Remove all the file views.  For a file which has multiple
// associated objects (i.e., an archive), we keep accessed views
// around until next time, in the hopes that they will be useful for
// the next object.

void
File_read::clear_views(bool destroying)
{
  Views::iterator p = this->views_.begin();
  while (p != this->views_.end())
    {
      bool should_delete;
      if (p->second->is_locked())
	should_delete = false;
      else if (destroying)
	should_delete = true;
      else if (p->second->should_cache())
	should_delete = false;
      else if (this->object_count_ > 1 && p->second->accessed())
	should_delete = false;
      else
	should_delete = true;

      if (should_delete)
	{
	  delete p->second;

	  // map::erase invalidates only the iterator to the deleted
	  // element.
	  Views::iterator pe = p;
	  ++p;
	  this->views_.erase(pe);
	}
      else
	{
	  gold_assert(!destroying);
	  p->second->clear_accessed();
	  ++p;
	}
    }

  Saved_views::iterator q = this->saved_views_.begin();
  while (q != this->saved_views_.end())
    {
      if (!(*q)->is_locked())
	{
	  delete *q;
	  q = this->saved_views_.erase(q);
	}
      else
	{
	  gold_assert(!destroying);
	  ++q;
	}
    }
}

// Print statistical information to stderr.  This is used for --stats.

void
File_read::print_stats()
{
  fprintf(stderr, _("%s: total bytes mapped for read: %llu\n"),
	  program_name, File_read::total_mapped_bytes);
  fprintf(stderr, _("%s: maximum bytes mapped for read at one time: %llu\n"),
	  program_name, File_read::maximum_mapped_bytes);
}

// Class File_view.

File_view::~File_view()
{
  gold_assert(this->file_.is_locked());
  this->view_->unlock();
}

// Class Input_file.

// Create a file for testing.

Input_file::Input_file(const Task* task, const char* name,
		       const unsigned char* contents, off_t size)
  : file_()
{
  this->input_argument_ =
    new Input_file_argument(name, false, "", false,
			    Position_dependent_options());
  bool ok = file_.open(task, name, contents, size);
  gold_assert(ok);
}

// Return the position dependent options in force for this file.

const Position_dependent_options&
Input_file::options() const
{
  return this->input_argument_->options();
}

// Return the name given by the user.  For -lc this will return "c".

const char*
Input_file::name() const
{
  return this->input_argument_->name();
}

// Return whether we are only reading symbols.

bool
Input_file::just_symbols() const
{
  return this->input_argument_->just_symbols();
}

// Open the file.

// If the filename is not absolute, we assume it is in the current
// directory *except* when:
//    A) input_argument_->is_lib() is true; or
//    B) input_argument_->extra_search_path() is not empty.
// In both cases, we look in extra_search_path + library_path to find
// the file location, rather than the current directory.

bool
Input_file::open(const General_options& options, const Dirsearch& dirpath,
		 const Task* task)
{
  std::string name;

  // Case 1: name is an absolute file, just try to open it
  // Case 2: name is relative but is_lib is false and extra_search_path
  //         is empty
  if (IS_ABSOLUTE_PATH (this->input_argument_->name())
      || (!this->input_argument_->is_lib()
	  && this->input_argument_->extra_search_path() == NULL))
    {
      name = this->input_argument_->name();
      this->found_name_ = name;
    }
  // Case 3: is_lib is true
  else if (this->input_argument_->is_lib())
    {
      // We don't yet support extra_search_path with -l.
      gold_assert(this->input_argument_->extra_search_path() == NULL);
      std::string n1("lib");
      n1 += this->input_argument_->name();
      std::string n2;
      if (options.is_static()
	  || !this->input_argument_->options().Bdynamic())
	n1 += ".a";
      else
	{
	  n2 = n1 + ".a";
	  n1 += ".so";
	}
      name = dirpath.find(n1, n2, &this->is_in_sysroot_);
      if (name.empty())
	{
	  gold_error(_("cannot find -l%s"),
		     this->input_argument_->name());
	  return false;
	}
      if (n2.empty() || name[name.length() - 1] == 'o')
	this->found_name_ = n1;
      else
	this->found_name_ = n2;
    }
  // Case 4: extra_search_path is not empty
  else
    {
      gold_assert(this->input_argument_->extra_search_path() != NULL);

      // First, check extra_search_path.
      name = this->input_argument_->extra_search_path();
      if (!IS_DIR_SEPARATOR (name[name.length() - 1]))
        name += '/';
      name += this->input_argument_->name();
      struct stat dummy_stat;
      if (::stat(name.c_str(), &dummy_stat) < 0)
        {
          // extra_search_path failed, so check the normal search-path.
          name = dirpath.find(this->input_argument_->name(), "",
			      &this->is_in_sysroot_);
          if (name.empty())
            {
              gold_error(_("cannot find %s"),
			 this->input_argument_->name());
	      return false;
            }
        }
      this->found_name_ = this->input_argument_->name();
    }

  // Now that we've figured out where the file lives, try to open it.

  General_options::Object_format format =
    this->input_argument_->options().format_enum();
  bool ok;
  if (format == General_options::OBJECT_FORMAT_ELF)
    ok = this->file_.open(task, name);
  else
    {
      gold_assert(format == General_options::OBJECT_FORMAT_BINARY);
      ok = this->open_binary(options, task, name);
    }

  if (!ok)
    {
      gold_error(_("cannot open %s: %s"),
		 name.c_str(), strerror(errno));
      return false;
    }

  return true;
}

// Open a file for --format binary.

bool
Input_file::open_binary(const General_options&,
			const Task* task, const std::string& name)
{
  // In order to open a binary file, we need machine code, size, and
  // endianness.  We may not have a valid target at this point, in
  // which case we use the default target.
  const Target* target;
  if (parameters->target_valid())
    target = &parameters->target();
  else
    target = &parameters->default_target();

  Binary_to_elf binary_to_elf(target->machine_code(),
			      target->get_size(),
			      target->is_big_endian(),
			      name);
  if (!binary_to_elf.convert(task))
    return false;
  return this->file_.open(task, name, binary_to_elf.converted_data_leak(),
			  binary_to_elf.converted_size());
}

} // End namespace gold.
