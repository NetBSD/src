/* Test view numbering zero-assert checking, same frag.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

	.file "dwarf2-8.c"
	.text
	.balign 4
	.globl _start
_start:
	.file 1 "dwarf2-8.c"
	.loc 1 1 view 0
	.loc 1 2 view 0
	.quad 0
	.size _start, .-_start
