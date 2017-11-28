/* Copyright (C) 2015-2017 Free Software Foundation, Inc.

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

#ifndef COMMON_ENUM_FLAGS_H
#define COMMON_ENUM_FLAGS_H

/* Type-safe wrapper for enum flags.  enum flags are enums where the
   values are bits that are meant to be ORed together.

   This allows writing code like the below, while with raw enums this
   would fail to compile without casts to enum type at the assignments
   to 'f':

    enum some_flag
    {
       flag_val1 = 1 << 1,
       flag_val2 = 1 << 2,
       flag_val3 = 1 << 3,
       flag_val4 = 1 << 4,
    };
    DEF_ENUM_FLAGS_TYPE(enum some_flag, some_flags)

    some_flags f = flag_val1 | flag_val2;
    f |= flag_val3;

   It's also possible to assign literal zero to an enum flags variable
   (meaning, no flags), dispensing adding an awkward explicit "no
   value" value to the enumeration.  For example:

    some_flags f = 0;
    f |= flag_val3 | flag_val4;

   Note that literal integers other than zero fail to compile:

    some_flags f = 1; // error
*/

#ifdef __cplusplus

/* Traits type used to prevent the global operator overloads from
   instantiating for non-flag enums.  */
template<typename T> struct enum_flags_type {};

/* Use this to mark an enum as flags enum.  It defines FLAGS as
   enum_flags wrapper class for ENUM, and enables the global operator
   overloads for ENUM.  */
#define DEF_ENUM_FLAGS_TYPE(enum_type, flags_type)	\
  typedef enum_flags<enum_type> flags_type;		\
  template<>						\
  struct enum_flags_type<enum_type>			\
  {							\
    typedef enum_flags<enum_type> type;			\
  }

/* Until we can rely on std::underlying type being universally
   available (C++11), roll our own for enums.  */
template<int size, bool sign> class integer_for_size { typedef void type; };
template<> struct integer_for_size<1, 0> { typedef uint8_t type; };
template<> struct integer_for_size<2, 0> { typedef uint16_t type; };
template<> struct integer_for_size<4, 0> { typedef uint32_t type; };
template<> struct integer_for_size<8, 0> { typedef uint64_t type; };
template<> struct integer_for_size<1, 1> { typedef int8_t type; };
template<> struct integer_for_size<2, 1> { typedef int16_t type; };
template<> struct integer_for_size<4, 1> { typedef int32_t type; };
template<> struct integer_for_size<8, 1> { typedef int64_t type; };

template<typename T>
struct enum_underlying_type
{
  typedef typename
    integer_for_size<sizeof (T), static_cast<bool>(T (-1) < T (0))>::type
    type;
};

template <typename E>
class enum_flags
{
public:
  typedef E enum_type;
  typedef typename enum_underlying_type<enum_type>::type underlying_type;

private:
  /* Private type used to support initializing flag types with zero:

       foo_flags f = 0;

     but not other integers:

       foo_flags f = 1;

     The way this works is that we define an implicit constructor that
     takes a pointer to this private type.  Since nothing can
     instantiate an object of this type, the only possible pointer to
     pass to the constructor is the NULL pointer, or, zero.  */
  struct zero_type;

  underlying_type
  underlying_value () const
  {
    return m_enum_value;
  }

public:
  /* Allow default construction.  */
  enum_flags ()
    : m_enum_value ((enum_type) 0)
  {}

  enum_flags (const enum_flags &other)
    : m_enum_value (other.m_enum_value)
  {}

  enum_flags &operator= (const enum_flags &other)
  {
    m_enum_value = other.m_enum_value;
    return *this;
  }

  /* If you get an error saying these two overloads are ambiguous,
     then you tried to mix values of different enum types.  */
  enum_flags (enum_type e)
    : m_enum_value (e)
  {}
  enum_flags (struct enum_flags::zero_type *zero)
    : m_enum_value ((enum_type) 0)
  {}

  enum_flags &operator&= (enum_type e)
  {
    m_enum_value = (enum_type) (underlying_value () & e);
    return *this;
  }
  enum_flags &operator|= (enum_type e)
  {
    m_enum_value = (enum_type) (underlying_value () | e);
    return *this;
  }
  enum_flags &operator^= (enum_type e)
  {
    m_enum_value = (enum_type) (underlying_value () ^ e);
    return *this;
  }

  operator enum_type () const
  {
    return m_enum_value;
  }

  enum_flags operator& (enum_type e) const
  {
    return (enum_type) (underlying_value () & e);
  }
  enum_flags operator| (enum_type e) const
  {
    return (enum_type) (underlying_value () | e);
  }
  enum_flags operator^ (enum_type e) const
  {
    return (enum_type) (underlying_value () ^ e);
  }
  enum_flags operator~ () const
  {
    return (enum_type) ~underlying_value ();
  }

private:
  /* Stored as enum_type because GDB knows to print the bit flags
     neatly if the enum values look like bit flags.  */
  enum_type m_enum_value;
};

/* Global operator overloads.  */

template <typename enum_type>
typename enum_flags_type<enum_type>::type
operator& (enum_type e1, enum_type e2)
{
  return enum_flags<enum_type> (e1) & e2;
}

template <typename enum_type>
typename enum_flags_type<enum_type>::type
operator| (enum_type e1, enum_type e2)
{
  return enum_flags<enum_type> (e1) | e2;
}

template <typename enum_type>
typename enum_flags_type<enum_type>::type
operator^ (enum_type e1, enum_type e2)
{
  return enum_flags<enum_type> (e1) ^ e2;
}

template <typename enum_type>
typename enum_flags_type<enum_type>::type
operator~ (enum_type e)
{
  return ~enum_flags<enum_type> (e);
}

#else /* __cplusplus */

/* In C, the flags type is just a typedef for the enum type.  */

#define DEF_ENUM_FLAGS_TYPE(enum_type, flags_type) \
  typedef enum_type flags_type

#endif /* __cplusplus */

#endif /* COMMON_ENUM_FLAGS_H */
