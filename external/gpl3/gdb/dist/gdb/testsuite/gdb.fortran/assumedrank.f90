! Copyright 2021-2023 Free Software Foundation, Inc.
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

!
! Start of test program.
!

PROGRAM  arank

  REAL :: array0
  REAL :: array1(10)
  REAL :: array2(1, 2)
  REAL :: array3(3, 4, 5)
  REAL :: array4(4, 5, 6, 7)

  array0 = 0
  array1 = 1.0
  array2 = 2.0
  array3 = 3.0
  array4 = 4.0

  call test_rank (array0)
  call test_rank (array1)
  call test_rank (array2)
  call test_rank (array3)
  call test_rank (array4)

  print *, "" ! Final Breakpoint

CONTAINS

  SUBROUTINE test_rank(answer)
    REAL :: answer(..)
    print *, RANK(answer)     ! Test Breakpoint
  END SUBROUTINE test_rank

END PROGRAM arank
