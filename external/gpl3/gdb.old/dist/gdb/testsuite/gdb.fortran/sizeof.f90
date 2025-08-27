! Copyright 2024 Free Software Foundation, Inc.

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

module data
  use, intrinsic :: iso_c_binding, only : C_SIZE_T
  implicit none

  character, target :: char_v
  character (len=3), target :: char_a
  integer, target :: int_v
  integer, target, dimension(:,:) :: int_2da (3,2)
  real*4, target :: real_v
  real*4, target :: real_a(4)
  real*4, target, dimension (:), allocatable :: real_a_alloc

  character, pointer :: char_v_p
  character (len=3), pointer :: char_a_p
  integer, pointer :: int_v_p
  integer, pointer, dimension (:,:) :: int_2da_p
  real*4, pointer :: real_v_p
  real*4, pointer, dimension(:) :: real_a_p
  real*4, dimension(:), pointer :: real_alloc_a_p

contains
subroutine test_sizeof (answer)
  integer(C_SIZE_T) :: answer

  print *, answer ! Test breakpoint
end subroutine test_sizeof

subroutine run_tests ()
  call test_sizeof (sizeof (char_v))
  call test_sizeof (sizeof (char_a))
  call test_sizeof (sizeof (int_v))
  call test_sizeof (sizeof (int_2da))
  call test_sizeof (sizeof (real_v))
  call test_sizeof (sizeof (real_a))
  call test_sizeof (sizeof (real_a_alloc))

  call test_sizeof (sizeof (char_v_p))
  call test_sizeof (sizeof (char_a_p))
  call test_sizeof (sizeof (int_v_p))
  call test_sizeof (sizeof (int_2da_p))
  call test_sizeof (sizeof (real_v_p))
  call test_sizeof (sizeof (real_a_p))
  call test_sizeof (sizeof (real_alloc_a_p))
end subroutine run_tests

end module data

program sizeof_tests
  use iso_c_binding
  use data

  implicit none

  allocate (real_a_alloc(5))

  nullify (char_v_p)
  nullify (char_a_p)
  nullify (int_v_p)
  nullify (int_2da_p)
  nullify (real_v_p)
  nullify (real_a_p)
  nullify (real_alloc_a_p)

  ! Test nullified
  call run_tests ()

  char_v_p => char_v ! Past unassigned pointers
  char_a_p => char_a
  int_v_p => int_v
  int_2da_p => int_2da
  real_v_p => real_v
  real_a_p => real_a
  real_alloc_a_p => real_a_alloc

  ! Test pointer assignment
  call run_tests ()

  char_v = 'a'
  char_a = "aaa"
  int_v = 10
  int_2da = reshape((/1, 2, 3, 4, 5, 6/), shape(int_2da))
  real_v = 123.123
  real_a_p = (/-1.1, -1.2, -1.3, -1.4/)
  real_a_alloc = (/1.1, 2.2, 3.3, 4.4, 5.5/)

  ! After allocate/value assignment
  call run_tests ()

  deallocate (real_a_alloc)

  print *, "done" ! Final breakpoint

end program sizeof_tests
