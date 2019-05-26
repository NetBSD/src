/* scoped_restore, a simple class for saving and restoring a value

   Copyright (C) 2016-2017 Free Software Foundation, Inc.

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

#ifndef SCOPED_RESTORE_H
#define SCOPED_RESTORE_H

/* Base class for scoped_restore_tmpl.  */
struct scoped_restore_base
{
};

/* A convenience typedef.  Users of make_scoped_restore declare the
   local RAII object as having this type.  */
typedef const scoped_restore_base &scoped_restore;

/* An RAII-based object that saves a variable's value, and then
   restores it again when this object is destroyed. */
template<typename T>
class scoped_restore_tmpl : public scoped_restore_base
{
 public:

  /* Create a new scoped_restore object that saves the current value
     of *VAR.  *VAR will be restored when this scoped_restore object
     is destroyed.  */
  scoped_restore_tmpl (T *var)
    : m_saved_var (var),
      m_saved_value (*var)
  {
  }

  /* Create a new scoped_restore object that saves the current value
     of *VAR, and sets *VAR to VALUE.  *VAR will be restored when this
     scoped_restore object is destroyed.  This is templated on T2 to
     allow passing VALUEs of types convertible to T.
     E.g.: T='base'; T2='derived'.  */
  template <typename T2>
  scoped_restore_tmpl (T *var, T2 value)
    : m_saved_var (var),
      m_saved_value (*var)
  {
    *var = value;
  }

  scoped_restore_tmpl (const scoped_restore_tmpl<T> &other)
    : m_saved_var (other.m_saved_var),
      m_saved_value (other.m_saved_value)
  {
    other.m_saved_var = NULL;
  }

  ~scoped_restore_tmpl ()
  {
    if (m_saved_var != NULL)
      *m_saved_var = m_saved_value;
  }

 private:

  /* No need for this.  It is intentionally not defined anywhere.  */
  scoped_restore_tmpl &operator= (const scoped_restore_tmpl &);

  /* The saved variable.  */
  mutable T *m_saved_var;

  /* The saved value.  */
  const T m_saved_value;
};

/* Make a scoped_restore.  This is useful because it lets template
   argument deduction work.  */
template<typename T>
scoped_restore_tmpl<T> make_scoped_restore (T *var)
{
  return scoped_restore_tmpl<T> (var);
}

/* Make a scoped_restore.  This is useful because it lets template
   argument deduction work.  */
template<typename T, typename T2>
scoped_restore_tmpl<T> make_scoped_restore (T *var, T2 value)
{
  return scoped_restore_tmpl<T> (var, value);
}

#endif /* SCOPED_RESTORE_H */
