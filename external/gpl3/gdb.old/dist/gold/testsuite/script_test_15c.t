/* script_test_15c.t -- linker script test 15c for gold

   Copyright (C) 2016-2020 Free Software Foundation, Inc.
   Written by Cary Coutant <ccoutant@google.com>.

   This file is part of gold.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* Test that an orphan .bss section is placed at the end of the segment.  */

/* We won't try to run this program, just ensure that it links
   as expected.  */

SECTIONS
{
  /* With luck this will be enough to get the program working.  */
  .interp : { *(.interp) }
  .text : { *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  /* Required by the ARM target. */
  .ARM.extab : { *(.ARM.extab*) }
  .ARM.exidx : { *(.ARM.exidx*) }
  . = ALIGN(0x10000);
  .dynamic : { *(.dynamic) }
  .data : { *(.data) }
  .got : { *(.got .toc) }
}
