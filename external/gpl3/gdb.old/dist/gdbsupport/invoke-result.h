/* Compatibility wrapper around std::result_of and std::invoke_result.

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

#ifndef GDBSUPPORT_INVOKE_RESULT_H
#define GDBSUPPORT_INVOKE_RESULT_H

#include <type_traits>

namespace gdb
{
#if __cplusplus >= 201703L
template<typename Callable, class... Args>
using invoke_result = std::invoke_result<Callable, Args...>;
#else
template<typename Callable, typename... Args>
using invoke_result = std::result_of<Callable (Args...)>;
#endif

} /* namespace gdb */

#endif /* GDBSUPPORT_INVOKE_RESULT_H */
