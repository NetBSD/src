! Copyright 2019-2024 Free Software Foundation, Inc.
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

program pointers

  type :: two
    integer, allocatable :: ivla1 (:)
    integer, allocatable :: ivla2 (:, :)
  end type two

  type :: typeWithPointer
    integer i
    type(typeWithPointer), pointer:: p
  end type typeWithPointer

  type :: twoPtr
    type (two), pointer :: p
  end type twoPtr

  logical, target :: logv
  complex, target :: comv
  character, target :: charv
  character (len=3), target :: chara
  integer, target :: intv
  integer, target, dimension (10,2) :: inta
  integer, target, allocatable, dimension (:) :: intvla
  real, target :: realv
  type(two), target :: twov
  type(twoPtr) :: arrayOfPtr (3)
  type(typeWithPointer), target:: cyclicp1,cyclicp2

  logical, pointer :: logp
  complex, pointer :: comp
  character, pointer :: charp
  character (len=3), pointer :: charap
  integer, pointer :: intp
  integer, pointer, dimension (:,:) :: intap
  integer, pointer, dimension (:) :: intvlap
  real, pointer :: realp
  type(two), pointer :: twop

  nullify (logp)
  nullify (comp)
  nullify (charp)
  nullify (charap)
  nullify (intp)
  nullify (intap)
  nullify (intvlap)
  nullify (realp)
  nullify (twop)
  nullify (arrayOfPtr(1)%p)
  nullify (arrayOfPtr(2)%p)
  nullify (arrayOfPtr(3)%p)
  nullify (cyclicp1%p)
  nullify (cyclicp2%p)

  logp => logv    ! Before pointer assignment
  comp => comv
  charp => charv
  charap => chara
  intp => intv
  intap => inta
  intvlap => intvla
  realp => realv
  twop => twov
  arrayOfPtr(2)%p => twov
  cyclicp1%i = 1
  cyclicp1%p => cyclicp2
  cyclicp2%i = 2
  cyclicp2%p => cyclicp1

  logv = associated(logp)     ! Before value assignment
  comv = cmplx(1,2)
  charv = "a"
  chara = "abc"
  intv = 10
  inta(:,:) = 1
  inta(3,1) = 3
  allocate (intvla(10))
  intvla(:) = 2
  intvla(4) = 4
  intvlap => intvla
  realv = 3.14

  allocate (twov%ivla1(3))
  allocate (twov%ivla2(2,2))
  twov%ivla1(1) = 11
  twov%ivla1(2) = 12
  twov%ivla1(3) = 13
  twov%ivla2(1,1) = 211
  twov%ivla2(2,1) = 221
  twov%ivla2(1,2) = 212
  twov%ivla2(2,2) = 222

  intv = intv + 1 ! After value assignment

end program pointers
