/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

namespace std
{

inline namespace __cxx11 {}

template <typename> struct allocator {};

template <class> struct char_traits;

inline namespace __cxx11 {

template <typename _CharT, typename = char_traits<_CharT>,
	  typename = allocator<_CharT>>

class basic_string;

} // namespace __cx11

typedef basic_string<char> string;

template <typename> struct allocator_traits;

template <typename _Tp> struct allocator_traits<allocator<_Tp>> {
  using pointer = _Tp *;
};

} // namespace std

struct __alloc_traits : std::allocator_traits<std::allocator<char>> {};

namespace std
{

inline namespace __cxx11
{

template <typename, typename, typename> struct basic_string
{
  typedef long size_type;

  size_type npos;

  struct _Alloc_hider
  {
    _Alloc_hider(__alloc_traits::pointer, const allocator<char> &);
  } _M_dataplus;

  basic_string(char *, allocator<char> __a = allocator<char>())
    : _M_dataplus(0, __a) {}
};

} // namespace __cxx11

} // namespace std

static char bar[] = "bar";

int
main (void)
{
  std::string foo = bar;
  return 0;
}
