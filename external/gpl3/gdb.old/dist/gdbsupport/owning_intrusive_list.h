/* Owning version of intrusive_list for GDB, the GNU debugger.
   Copyright (C) 2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_OWNING_INTRUSIVE_LIST_H
#define GDBSUPPORT_OWNING_INTRUSIVE_LIST_H

#include "intrusive_list.h"

/* An owning version of intrusive_list.  */

template<typename T, typename AsNode = intrusive_base_node<T>>
class owning_intrusive_list : private intrusive_list<T, AsNode>
{
  using base = intrusive_list<T, AsNode>;

public:
  using value_type = typename base::value_type;
  using reference = typename base::reference;
  using iterator = typename base::iterator;
  using reverse_iterator = typename base::reverse_iterator;
  using const_iterator = typename base::const_iterator;
  using unique_pointer = std::unique_ptr<T>;

  using base::iterator_to;
  using base::front;
  using base::back;
  using base::empty;
  using base::begin;
  using base::cbegin;
  using base::end;
  using base::cend;
  using base::rbegin;
  using base::crbegin;
  using base::rend;
  using base::crend;

  owning_intrusive_list () noexcept = default;

  owning_intrusive_list (owning_intrusive_list &&other) noexcept
    : base (std::move (other))
  {
  }

  ~owning_intrusive_list ()
  { this->clear (); }

  owning_intrusive_list &operator= (owning_intrusive_list &&other) noexcept
  {
    this->clear ();
    this->base::operator= (std::move (other));
    return *this;
  }

  void swap (owning_intrusive_list &other) noexcept
  { this->base::swap (other); }

  /* Insert ELEM at the front of the list.

     The list takes ownership of ELEM.  */
  void push_front (unique_pointer elem) noexcept
  { this->base::push_front (*elem.release ()); }

  /* Insert ELEM at the back of the list.

     The list takes ownership of ELEM.  */
  void push_back (unique_pointer elem) noexcept
  { this->base::push_back (*elem.release ()); }

  /* Insert ELEM before POS in the list.

     The list takes ownership of ELEM.  */
  iterator insert (const_iterator pos, unique_pointer elem) noexcept
  { return  this->base::insert (pos, *elem.release ());  }

  void splice (owning_intrusive_list &&other) noexcept
  { this->base::splice (std::move (other)); }

  /* Remove the element at the front of the list.  The element is destroyed.  */
  void pop_front () noexcept
  {
    unique_pointer holder (&this->front ());
    this->base::pop_front ();
  }

  /* Remove the element at the back of the list.  The element is destroyed.  */
  void pop_back () noexcept
  {
    unique_pointer holder (&this->back ());
    this->base::pop_back ();
  }

  /* Remove the element pointed by I from the list.  The element
     pointed by I is destroyed.  */
  iterator erase (const_iterator i) noexcept
  {
    unique_pointer holder (&*i);
    return this->base::erase (i);
  }

  /* Remove all elements from the list.  All elements are destroyed.  */
  void clear () noexcept
  {
    while (!this->empty ())
      this->pop_front ();
  }

  /* Construct an element in-place at the front of the list.

     Return a reference to the new element.  */
  template<typename... Args>
  reference emplace_front (Args &&...args)
  { return this->emplace (this->begin (), std::forward<Args> (args)...); }

  /* Construct an element in-place at the back of the list.

     Return a reference to the new element.  */
  template<typename... Args>
  reference emplace_back (Args &&...args)
  { return this->emplace (this->end (), std::forward<Args> (args)...); }

  /* Construct an element in-place in the list, before POS.

     Return a reference to the new element.  */
  template<typename... Args>
  reference emplace (const_iterator pos, Args &&...args)
  {
    return *this->insert (pos,
			 std::make_unique<T> (std::forward<Args> (args)...));
  }

  /* Return type for the release method.  */
  struct release_ret
  {
    /* Iterator to the following element in the list.  */
    iterator next;

    /* The released element.  */
    unique_pointer released;
  };

  release_ret release (const_iterator i) noexcept
  {
    iterator next = i;
    ++next;
    unique_pointer released (&*i);

    this->unlink_element (*i);

    return { next, std::move (released) };
  }
};

#endif /* GDBSUPPORT_OWNING_INTRUSIVE_LIST_H */
