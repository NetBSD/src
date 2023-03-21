#!/bin/sh

# ver_test_14.sh -- a test case for version scripts

# Copyright (C) 2018-2020 Free Software Foundation, Inc.
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

check()
{
    if ! grep -q "$2" "$1"
    then
	echo "Did not find expected symbol in $1:"
	echo "   $2"
	echo ""
	echo "Actual output below:"
	cat "$1"
	exit 1
    fi
}

check ver_test_14.syms "V1 *\(0x[0-9a-f][048c]\)\? t2()$"
check ver_test_14.syms "V1 *\(0x[0-9a-f][048c]\)\? t3()$"
check ver_test_14.syms "V1 *\(0x[0-9a-f][048c]\)\? t4()$"
check ver_test_14.syms "Base *\(0x[0-9a-f][048c]\)\? t4_2a$"

exit 0
