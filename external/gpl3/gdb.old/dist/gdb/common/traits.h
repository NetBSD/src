/* Copyright (C) 2017 Free Software Foundation, Inc.

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

#ifndef COMMON_TRAITS_H
#define COMMON_TRAITS_H

namespace gdb {

/* Pre C++14-safe (CWG 1558) version of C++17's std::void_t.  See
   <http://en.cppreference.com/w/cpp/types/void_t>.  */

template<typename... Ts>
struct make_void { typedef void type; };

template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

}

#endif /* COMMON_TRAITS_H */
