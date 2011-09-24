! Copyright 2010, 2011 Free Software Foundation, Inc.
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 2 of the License, or
! (at your option) any later version.
!
! This program is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.
!
! You should have received a copy of the GNU General Public License
! along with this program; if not, write to the Free Software
! Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
!
! This file was written by Jan Kratochvil <jan.kratochvil@redhat.com>.

program test
  logical :: l
  logical (kind=1) :: l1
  logical (kind=2) :: l2
  logical (kind=4) :: l4
  logical (kind=8) :: l8
  l = .TRUE.
  l1 = .TRUE.
  l2 = .TRUE.
  l4 = .TRUE.
  l8 = .TRUE.
  l = .FALSE.					! stop-here
end
