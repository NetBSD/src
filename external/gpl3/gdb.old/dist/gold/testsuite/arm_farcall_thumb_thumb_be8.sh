#!/bin/sh

# arm_farcall_thumb_thumb_be8.sh -- a test case for Thumb->Thumb farcall veneers.

# Copyright (C) 2010-2020 Free Software Foundation, Inc.
# This file is part of gold.
# Based on arm_farcall_thumb_thumb.s file

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
	echo "Did not find expected instruction in $1:"
	echo "   $2"
	echo ""
	echo "Actual instructions below:"
	cat "$1"
	exit 1
    fi
}

# Thumb->Thumb
check arm_farcall_thumb_thumb_be8.stdout "1004:\sb401"
check arm_farcall_thumb_thumb_be8.stdout "1006:\s4802"
check arm_farcall_thumb_thumb_be8.stdout "1008:\s4684"
check arm_farcall_thumb_thumb_be8.stdout "100a:\sbc01"
check arm_farcall_thumb_thumb_be8.stdout "100c:\s4760"
check arm_farcall_thumb_thumb_be8.stdout "100e:\sbf00"
check arm_farcall_thumb_thumb_be8.stdout "1010:\s0002"
check arm_farcall_thumb_thumb_be8.stdout "1012:\s1510"

exit 0
