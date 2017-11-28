! Copyright 2016 Free Software Foundation, Inc.
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

program TestNestedFuncs

  IMPLICIT NONE

  TYPE :: t_State
    integer :: code
  END TYPE t_State

  TYPE (t_State) :: v_state
  integer index

  index = 13
  CALL sub_nested_outer
  index = 11              ! BP_main
  v_state%code = 27

CONTAINS

  SUBROUTINE sub_nested_outer
    integer local_int
    local_int = 19
    v_state%code = index + local_int   ! BP_outer
    call sub_nested_inner
    local_int = 22                     ! BP_outer_2
    RETURN
  END SUBROUTINE sub_nested_outer

  SUBROUTINE sub_nested_inner
    integer local_int
    local_int = 17
    v_state%code = index + local_int   ! BP_inner
    RETURN
  END SUBROUTINE sub_nested_inner

end program TestNestedFuncs
