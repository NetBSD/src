! Copyright 2020-2023 Free Software Foundation, Inc.
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 3 of the License, or
! (at your option) any later version.
!
! This program is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.
!
! You should have received a copy of the GNU General Public License
! along with this program.  If not, see <http://www.gnu.org/licenses/>.

! Return ARG plus 1.
integer function some_func (arg)
  integer :: arg

  some_func = (arg + 1)
end function some_func

! Print STR.
integer function string_func (str)
  character(len=*) :: str

  print *, str
  string_func = 0
end function string_func
