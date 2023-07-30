/* Frame info pointer

   Copyright (C) 2022-2023 Free Software Foundation, Inc.

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

#ifndef GDB_FRAME_INFO_H
#define GDB_FRAME_INFO_H

#include "gdbsupport/intrusive_list.h"
#include "frame-id.h"

struct frame_info;

/* A wrapper for "frame_info *".  frame_info objects are invalidated
   whenever reinit_frame_cache is called.  This class arranges to
   invalidate the pointer when appropriate.  This is done to help
   detect a GDB bug that was relatively common.

   A small amount of code must still operate on raw pointers, so a
   "get" method is provided.  However, you should normally not use
   this in new code.  */

class frame_info_ptr : public intrusive_list_node<frame_info_ptr>
{
public:
  /* Create a frame_info_ptr from a raw pointer.  */
  explicit frame_info_ptr (struct frame_info *ptr)
    : m_ptr (ptr)
  {
    frame_list.push_back (*this);
  }

  /* Create a null frame_info_ptr.  */
  frame_info_ptr ()
  {
    frame_list.push_back (*this);
  }

  frame_info_ptr (std::nullptr_t)
  {
    frame_list.push_back (*this);
  }

  frame_info_ptr (const frame_info_ptr &other)
    : m_ptr (other.m_ptr),
      m_cached_id (other.m_cached_id),
      m_cached_level (other.m_cached_level)
  {
    frame_list.push_back (*this);
  }

  frame_info_ptr (frame_info_ptr &&other)
    : m_ptr (other.m_ptr),
      m_cached_id (other.m_cached_id),
      m_cached_level (other.m_cached_level)
  {
    other.m_ptr = nullptr;
    other.m_cached_id = null_frame_id;
    other.m_cached_level = invalid_level;
    frame_list.push_back (*this);
  }

  ~frame_info_ptr ()
  {
    /* If this node has static storage, it may be deleted after
       frame_list.  Attempting to erase ourselves would then trigger
       internal errors, so make sure we are still linked first.  */
    if (is_linked ())
      frame_list.erase (frame_list.iterator_to (*this));
  }

  frame_info_ptr &operator= (const frame_info_ptr &other)
  {
    m_ptr = other.m_ptr;
    m_cached_id = other.m_cached_id;
    m_cached_level = other.m_cached_level;
    return *this;
  }

  frame_info_ptr &operator= (std::nullptr_t)
  {
    m_ptr = nullptr;
    m_cached_id = null_frame_id;
    m_cached_level = invalid_level;
    return *this;
  }

  frame_info_ptr &operator= (frame_info_ptr &&other)
  {
    m_ptr = other.m_ptr;
    m_cached_id = other.m_cached_id;
    m_cached_level = other.m_cached_level;
    other.m_ptr = nullptr;
    other.m_cached_id = null_frame_id;
    other.m_cached_level = invalid_level;
    return *this;
  }

  frame_info *operator-> () const
  {
    return m_ptr;
  }

  /* Fetch the underlying pointer.  Note that new code should
     generally not use this -- avoid it if at all possible.  */
  frame_info *get () const
  {
    return m_ptr;
  }

  /* This exists for compatibility with pre-existing code that checked
     a "frame_info *" using "!".  */
  bool operator! () const
  {
    return m_ptr == nullptr;
  }

  /* This exists for compatibility with pre-existing code that checked
     a "frame_info *" like "if (ptr)".  */
  explicit operator bool () const
  {
    return m_ptr != nullptr;
  }

  /* Invalidate this pointer.  */
  void invalidate ()
  {
    m_ptr = nullptr;
  }

  /* Cache the frame_id that the pointer will use to reinflate.  */
  void prepare_reinflate ();

  /* Use the cached frame_id to reinflate the pointer.  */
  void reinflate ();

private:
  /* We sometimes need to construct frame_info_ptr objects around the
     sentinel_frame, which has level -1.  Therefore, make the invalid frame
     level value -2.  */
  static constexpr int invalid_level = -2;

  /* The underlying pointer.  */
  frame_info *m_ptr = nullptr;

  /* The frame_id of the underlying pointer.  */
  frame_id m_cached_id = null_frame_id;

  /* The frame level of the underlying pointer.  */
  int m_cached_level = invalid_level;

  /* All frame_info_ptr objects are kept on an intrusive list.
     This keeps their construction and destruction costs
     reasonably small.  */
  static intrusive_list<frame_info_ptr> frame_list;

  /* A friend so it can invalidate the pointers.  */
  friend void reinit_frame_cache ();
};

static inline bool
operator== (const frame_info *self, const frame_info_ptr &other)
{
  return self == other.get ();
}

static inline bool
operator== (const frame_info_ptr &self, const frame_info_ptr &other)
{
  return self.get () == other.get ();
}

static inline bool
operator== (const frame_info_ptr &self, const frame_info *other)
{
  return self.get () == other;
}

static inline bool
operator!= (const frame_info *self, const frame_info_ptr &other)
{
  return self != other.get ();
}

static inline bool
operator!= (const frame_info_ptr &self, const frame_info_ptr &other)
{
  return self.get () != other.get ();
}

static inline bool
operator!= (const frame_info_ptr &self, const frame_info *other)
{
  return self.get () != other;
}

#endif /* GDB_FRAME_INFO_H */
