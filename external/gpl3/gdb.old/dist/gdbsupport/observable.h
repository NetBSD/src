/* Observers

   Copyright (C) 2016-2020 Free Software Foundation, Inc.

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

#ifndef COMMON_OBSERVABLE_H
#define COMMON_OBSERVABLE_H

#include <algorithm>
#include <functional>
#include <vector>

namespace gdb
{

namespace observers
{

extern unsigned int observer_debug;

/* An observer is an entity which is interested in being notified
   when GDB reaches certain states, or certain events occur in GDB.
   The entity being observed is called the observable.  To receive
   notifications, the observer attaches a callback to the observable.
   One observable can have several observers.

   The observer implementation is also currently not reentrant.  In
   particular, it is therefore not possible to call the attach or
   detach routines during a notification.  */

/* The type of a key that can be passed to attach, which can be passed
   to detach to remove associated observers.  Tokens have address
   identity, and are thus usually const globals.  */
struct token
{
  token () = default;

  DISABLE_COPY_AND_ASSIGN (token);
};

template<typename... T>
class observable
{
public:

  typedef std::function<void (T...)> func_type;

  explicit observable (const char *name)
    : m_name (name)
  {
  }

  DISABLE_COPY_AND_ASSIGN (observable);

  /* Attach F as an observer to this observable.  F cannot be
     detached.  */
  void attach (const func_type &f)
  {
    m_observers.emplace_back (nullptr, f);
  }

  /* Attach F as an observer to this observable.  T is a reference to
     a token that can be used to later remove F.  */
  void attach (const func_type &f, const token &t)
  {
    m_observers.emplace_back (&t, f);
  }

  /* Remove observers associated with T from this observable.  T is
     the token that was previously passed to any number of "attach"
     calls.  */
  void detach (const token &t)
  {
    auto iter = std::remove_if (m_observers.begin (),
				m_observers.end (),
				[&] (const std::pair<const token *,
				     func_type> &e)
				{
				  return e.first == &t;
				});

    m_observers.erase (iter, m_observers.end ());
  }

  /* Notify all observers that are attached to this observable.  */
  void notify (T... args) const
  {
    if (observer_debug)
      fprintf_unfiltered (gdb_stdlog, "observable %s notify() called\n",
			  m_name);
    for (auto &&e : m_observers)
      e.second (args...);
  }

private:

  std::vector<std::pair<const token *, func_type>> m_observers;
  const char *m_name;
};

} /* namespace observers */

} /* namespace gdb */

#endif /* COMMON_OBSERVABLE_H */
