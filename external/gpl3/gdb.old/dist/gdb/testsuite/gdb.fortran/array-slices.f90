! Copyright 2019-2020 Free Software Foundation, Inc.
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

subroutine show (message, array)
  character (len=*) :: message
  integer, dimension (:,:) :: array

  print *, message
  do i=LBOUND (array, 2), UBOUND (array, 2), 1
     do j=LBOUND (array, 1), UBOUND (array, 1), 1
        write(*, fmt="(i4)", advance="no") array (j, i)
     end do
     print *, ""
 end do
 print *, array
 print *, ""

end subroutine show

program test

  interface
     subroutine show (message, array)
       character (len=*) :: message
       integer, dimension(:,:) :: array
     end subroutine show
  end interface

  integer, dimension (1:10,1:10) :: array
  integer, allocatable :: other (:, :)

  allocate (other (-5:4, -2:7))

  do i=LBOUND (array, 2), UBOUND (array, 2), 1
     do j=LBOUND (array, 1), UBOUND (array, 1), 1
        array (j,i) = ((i - 1) * UBOUND (array, 2)) + j
     end do
  end do

  do i=LBOUND (other, 2), UBOUND (other, 2), 1
     do j=LBOUND (other, 1), UBOUND (other, 1), 1
        other (j,i) = ((i - 1) * UBOUND (other, 2)) + j
     end do
  end do

  call show ("array", array)
  call show ("array (1:5,1:5)", array (1:5,1:5))
  call show ("array (1:10:2,1:10:2)", array (1:10:2,1:10:2))
  call show ("array (1:10:3,1:10:2)", array (1:10:3,1:10:2))
  call show ("array (1:10:5,1:10:3)", array (1:10:4,1:10:3))

  call show ("other", other)
  call show ("other (-5:0, -2:0)", other (-5:0, -2:0))
  call show ("other (-5:4:2, -2:7:3)", other (-5:4:2, -2:7:3))

  deallocate (other)
  print *, "" ! Final Breakpoint.
end program test
