// gold-threads.h -- thread support for gold  -*- C++ -*-

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

// gold can be configured to support threads.  If threads are
// supported, the user can specify at runtime whether or not to
// support them.  This provides an interface to manage locking
// accordingly.

// Lock
//   A simple lock class.

#ifndef GOLD_THREADS_H
#define GOLD_THREADS_H

namespace gold
{

class Condvar;

// The interface for the implementation of a Lock.

class Lock_impl
{
 public:
  Lock_impl()
  { }

  virtual
  ~Lock_impl()
  { }

  virtual void
  acquire() = 0;

  virtual void
  release() = 0;
};

// A simple lock class.

class Lock
{
 public:
  Lock();

  ~Lock();

  // Acquire the lock.
  void
  acquire()
  { this->lock_->acquire(); }

  // Release the lock.
  void
  release()
  { this->lock_->release(); }

 private:
  // This class can not be copied.
  Lock(const Lock&);
  Lock& operator=(const Lock&);

  friend class Condvar;
  Lock_impl*
  get_impl() const
  { return this->lock_; }

  Lock_impl* lock_;
};

// RAII for Lock.

class Hold_lock
{
 public:
  Hold_lock(Lock& lock)
    : lock_(lock)
  { this->lock_.acquire(); }

  ~Hold_lock()
  { this->lock_.release(); }

 private:
  // This class can not be copied.
  Hold_lock(const Hold_lock&);
  Hold_lock& operator=(const Hold_lock&);

  Lock& lock_;
};

class Hold_optional_lock
{
 public:
  Hold_optional_lock(Lock* lock)
    : lock_(lock)
  {
    if (this->lock_ != NULL)
      this->lock_->acquire();
  }

  ~Hold_optional_lock()
  {
    if (this->lock_ != NULL)
      this->lock_->release();
  }

 private:
  Hold_optional_lock(const Hold_optional_lock&);
  Hold_optional_lock& operator=(const Hold_optional_lock&);

  Lock* lock_;
};

// The interface for the implementation of a condition variable.

class Condvar_impl
{
 public:
  Condvar_impl()
  { }

  virtual
  ~Condvar_impl()
  { }

  virtual void
  wait(Lock_impl*) = 0;

  virtual void
  signal() = 0;

  virtual void
  broadcast() = 0;
};

// A simple condition variable class.  It is always associated with a
// specific lock.

class Condvar
{
 public:
  Condvar(Lock& lock);
  ~Condvar();

  // Wait for the condition variable to be signalled.  This should
  // only be called when the lock is held.
  void
  wait()
  { this->condvar_->wait(this->lock_.get_impl()); }

  // Signal the condition variable--wake up at least one thread
  // waiting on the condition variable.  This should only be called
  // when the lock is held.
  void
  signal()
  { this->condvar_->signal(); }

  // Broadcast the condition variable--wake up all threads waiting on
  // the condition variable.  This should only be called when the lock
  // is held.
  void
  broadcast()
  { this->condvar_->broadcast(); }

 private:
  // This class can not be copied.
  Condvar(const Condvar&);
  Condvar& operator=(const Condvar&);

  Lock& lock_;
  Condvar_impl* condvar_;
};

} // End namespace gold.

#endif // !defined(GOLD_THREADS_H)
