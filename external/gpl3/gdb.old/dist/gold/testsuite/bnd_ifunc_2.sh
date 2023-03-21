#!/bin/sh

# bnd_ifunc_2.sh -- test -z bndplt for x86_64

# Copyright (C) 2016-2020 Free Software Foundation, Inc.
# Written by Cary Coutant <ccoutant@gmail.com>.

# This file is part of gold.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.

match()
{
  if ! egrep "$1" "$2" >/dev/null 2>&1; then
    echo 1>&2 "could not find '$1' in $2"
    exit 1
  fi
}

# Extract just the PLT portion of the disassembly.
get_plt()
{
  sed -n -e '/^Disassembly of section .plt:/,/^Disassembly/p'
}

# Extract the addresses of the indirect jumps, omitting the PLT0 entry.
get_aplt_jmpq_addresses()
{
  sed -n -e '/_GLOBAL_OFFSET_TABLE_+0x10/d' \
	 -e '/bnd jmpq \*0x[0-9a-f]*(%rip)/p' |
    sed -e 's/ *\([0-9a-f]*\):.*/\1/'
}

for APLT_ADDR in $(get_plt < bnd_ifunc_2.stdout | get_aplt_jmpq_addresses)
do
  match "bnd (callq|jmpq) $APLT_ADDR" bnd_ifunc_2.stdout
done
