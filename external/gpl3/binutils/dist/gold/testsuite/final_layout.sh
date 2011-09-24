#!/bin/sh

# final_layout.sh -- test --final-layout

# Copyright 2010 Free Software Foundation, Inc.
# Written by Sriraman Tallam <tmsriram@google.com>.

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

# The goal of this program is to verify if --section-ordering-file works as
# intended.  File final_layout.cc is in this test.

check()
{
    func_addr_1=`grep $2 $1 | awk '{print $1}' | tr 'abcdef' 'ABCDEF'`
    func_addr_1=`echo 16i${func_addr_1}p | dc`
    func_addr_2=`grep $3 $1 | awk '{print $1}' | tr 'abcdef' 'ABCDEF'`
    func_addr_2=`echo 16i${func_addr_2}p | dc`
    if [ $func_addr_1 -gt $func_addr_2 ]
    then
        echo "final layout of" $2 "and" $3 "is not right."
	exit 1
    fi
}

check final_layout.stdout "_Z3barv" "_Z3bazv"
check final_layout.stdout "_Z3bazv" "_Z3foov"
check final_layout.stdout "global_varb" "global_vara"
check final_layout.stdout "global_vara" "global_varc"
