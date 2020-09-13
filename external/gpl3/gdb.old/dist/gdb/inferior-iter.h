/* Inferior iterators and ranges for GDB, the GNU debugger.

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

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

#ifndef INFERIOR_ITER_H
#define INFERIOR_ITER_H

#include "common/filtered-iterator.h"
#include "common/safe-iterator.h"

/* A forward iterator that iterates over all inferiors.  */

class all_inferiors_iterator
{
public:
  typedef all_inferiors_iterator self_type;
  typedef struct inferior *value_type;
  typedef struct inferior *&reference;
  typedef struct inferior **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  /* Create an iterator pointing at HEAD.  */
  explicit all_inferiors_iterator (inferior *head)
    : m_inf (head)
  {}

  /* Create a one-past-end iterator.  */
  all_inferiors_iterator ()
    : m_inf (nullptr)
  {}

  all_inferiors_iterator &operator++ ()
  {
    m_inf = m_inf->next;
    return *this;
  }

  inferior *operator* () const
  { return m_inf; }

  bool operator!= (const all_inferiors_iterator &other) const
  { return m_inf != other.m_inf; }

private:
  inferior *m_inf;
};

/* Filter for filtered_iterator.  Filters out exited inferiors.  */

struct exited_inferior_filter
{
  bool operator() (inferior *inf)
  {
    return inf->pid != 0;
  }
};

/* Iterate over all non-exited inferiors.  */

using all_non_exited_inferiors_iterator
  = filtered_iterator<all_inferiors_iterator, exited_inferior_filter>;

/* A range adapter that makes it possible to iterate over all
   inferiors with range-for.  */
struct all_inferiors_range
{
  all_inferiors_iterator begin () const
  { return all_inferiors_iterator (inferior_list); }
  all_inferiors_iterator end () const
  { return all_inferiors_iterator (); }
};

/* Iterate over all inferiors, safely.  */

using all_inferiors_safe_iterator
  = basic_safe_iterator<all_inferiors_iterator>;

/* A range adapter that makes it possible to iterate over all
   inferiors with range-for "safely".  I.e., it is safe to delete the
   currently-iterated inferior.  */

struct all_inferiors_safe_range
{
  all_inferiors_safe_iterator begin () const
  { return all_inferiors_safe_iterator (inferior_list); }
  all_inferiors_safe_iterator end () const
  { return all_inferiors_safe_iterator (); }
};

/* A range adapter that makes it possible to iterate over all
   non-exited inferiors with range-for.  */

struct all_non_exited_inferiors_range
{
  all_non_exited_inferiors_iterator begin () const
  { return all_non_exited_inferiors_iterator (inferior_list); }
  all_non_exited_inferiors_iterator end () const
  { return all_non_exited_inferiors_iterator (); }
};

#endif /* !defined (INFERIOR_ITER_H) */
