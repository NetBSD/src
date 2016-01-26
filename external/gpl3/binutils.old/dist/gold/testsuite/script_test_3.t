/* script_test_3.t -- linker script test 3 for gold

   Copyright 2008, 2010 Free Software Foundation, Inc.
   Written by Ian Lance Taylor <iant@google.com>.

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

SECTIONS
{
  /* With luck this will work everywhere.  */
  . = 0x10000000;

  /* With luck this will be enough to get the program working.  */
  .interp : { *(.interp) } :text :interp
  .text : { *(.text) } :text
  /* Required by the ARM target. */
  .ARM.extab : { *(.ARM.extab*) }
  .ARM.exidx : { *(.ARM.exidx*) }
  . += 0x100000;
  . = ALIGN(0x100);
  .dynamic : { *(.dynamic) } :data :dynamic
  .data : { *(.data) } :data
  .tdata : { *(.tdata*) } :data :tls
  .tbss : { *(.tbss*) } :data :tls
  . += 0x100000;
  . = ALIGN(0x100000);
  .bss : { *(.bss) } :bss
}

PHDRS
{
  text PT_LOAD FILEHDR PHDRS FLAGS(5);
  interp PT_INTERP;
  dynamic PT_DYNAMIC FLAGS(4);
  data PT_LOAD;
  bss PT_LOAD;
  tls PT_TLS;
}
