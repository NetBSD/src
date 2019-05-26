! Copyright 2018-2019 Free Software Foundation, Inc.
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
! along with this program.  If not, see <http://www.gnu.org/licenses/> .

! Source code for short-circuit-argument-list.exp.

logical function function_no_arg()
    print *, "No, return true."
    function_no_arg = .TRUE.
end function function_no_arg

logical function function_no_arg_false()
    function_no_arg_false = .FALSE.
end function function_no_arg_false

logical function function_one_arg(x)
    logical, intent(in) :: x
    print *, "One, return true."
    function_one_arg = .TRUE.
end function function_one_arg

logical function function_two_arg(x, y)
    logical, intent(in) :: x, y
    print *, "Two, return true."
    function_two_arg = .TRUE.
end function function_two_arg

logical function function_array(logical_array)
    logical, dimension(4,2), target, intent(in) :: logical_array
    logical, dimension(:,:), pointer :: p
    p => logical_array
    print *, "Array, return true.", p(1,1), logical_array(1,1)
    function_array = .TRUE.
end function function_array

program generate_truth_table
    implicit none
    interface
	logical function function_no_arg()
	end function function_no_arg
	logical function function_no_arg_false()
	end function
	logical function function_one_arg(x)
	    logical, intent(in) :: x
	end function
	logical function function_two_arg(x, y)
	    logical, intent(in) :: x, y
	end function
	logical function function_array(logical_array)
	    logical, dimension(4,2), target, intent(in) :: logical_array
	end function function_array
    end interface
    logical, dimension (4,2) :: truth_table
    logical :: a, b, c, d, e
    character(2) :: binary_string
    binary_string = char(0) // char(1)
    truth_table = .FALSE.
    truth_table(3:4,1) = .TRUE.
    truth_table(2::2,2) = .TRUE.
    a = function_no_arg() ! post_truth_table_init
    b = function_no_arg_false()
    c = function_one_arg(b)
    d = function_two_arg(a, b)
    e = function_array(truth_table)
    print *, truth_table(:, 1), a, b, e
    print *, truth_table(:, 2), c, d
end program generate_truth_table
