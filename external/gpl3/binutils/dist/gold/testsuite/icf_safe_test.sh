# icf_safe_test.sh -- test --icf=safe

# Copyright 2009 Free Software Foundation, Inc.
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

# The goal of this program is to verify if --icf=safe  works as expected.
# File icf_safe_test.cc is in this test. This program checks if only
# ctors and dtors are folded, except for x86 (32 and 64 bit), which
# uses relocation types to detect if function pointers are taken.

check_nofold()
{
    func_addr_1=`grep $2 $1 | awk '{print $1}'`
    func_addr_2=`grep $3 $1 | awk '{print $1}'`
    if [ $func_addr_1 = $func_addr_2 ]
    then
        echo "Safe Identical Code Folding folded" $2 "and" $3
	exit 1
    fi
}

check_fold()
{
    func_addr_1=`grep $2 $1 | awk '{print $1}'`
    func_addr_2=`grep $3 $1 | awk '{print $1}'`
    if [ $func_addr_1 != $func_addr_2 ]
    then
        echo "Safe Identical Code Folding did not fold " $2 "and" $3
	exit 1
    fi
}

arch_specific_safe_fold()
{
    grep_x86=`grep -q -e "Advanced Micro Devices X86-64" -e "Intel 80386" -e "ARM" $2`
    if [ $? == 0 ];
    then
      check_fold $1 $3 $4
    else
      check_nofold $1 $3 $4
    fi
}

arch_specific_safe_fold icf_safe_test_1.stdout icf_safe_test_2.stdout \
 "kept_func_1" "kept_func_2"
check_fold   icf_safe_test_1.stdout "_ZN1AD1Ev" "_ZN1AC1Ev"
check_nofold icf_safe_test_1.stdout "kept_func_3" "kept_func_1"
check_nofold icf_safe_test_1.stdout "kept_func_3" "kept_func_2"
