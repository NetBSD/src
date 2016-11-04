#!/bin/sh

# icf_safe_so_test.sh -- test --icf=safe

# Copyright (C) 2010-2015 Free Software Foundation, Inc.
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
# File icf_safe_so_test.cc is in this test.  The goal of this script is
# to verify if identical code folding in safe mode correctly folds
# functions in a shared object.

error_if_symbol_absent()
{
    if ! is_symbol_present $1 $2;
    then
      echo "Symbol" $2 "not present, possibly folded."
      exit 1
    fi
}

is_symbol_present()
{
    grep $2 $1 > /dev/null 2>&1
    return $?
}

check_nofold()
{
    error_if_symbol_absent $1 $2
    error_if_symbol_absent $1 $3
    func_addr_1=`grep $2 $1 | awk '{print $1}'`
    func_addr_2=`grep $3 $1 | awk '{print $1}'`
    if [ $func_addr_1 = $func_addr_2 ];
    then
        echo "Safe Identical Code Folding folded" $2 "and" $3
	exit 1
    fi
}

check_fold()
{
    if ! is_symbol_present $1 $2
    then
      return 0
    fi

    if ! is_symbol_present $1 $3
    then
      return 0
    fi

    awk "
BEGIN { discard = 0; }
/^Discarded input/ { discard = 1; }
/^Memory map/ { discard = 0; }
/.*\\.text\\..*($2|$3).*/ { act[discard] = act[discard] \" \" \$0; }
END {
      # printf \"kept\" act[0] \"\\nfolded\" act[1] \"\\n\";
      if (length(act[0]) == 0 || length(act[1]) == 0)
	{
	  printf \"Safe Identical Code Folding did not fold $2 and $3\\n\"
	  exit 1;
	}
    }" $4
}

arch_specific_safe_fold()
{
    grep -e "Intel 80386" -e "ARM" -e "PowerPC" $1 > /dev/null 2>&1
    if [ $? -eq 0 ];
    then
      check_fold $2 $4 $5 $3
    else
      check_nofold $2 $4 $5
    fi
}

arch_specific_safe_fold icf_safe_so_test_2.stdout icf_safe_so_test_1.stdout icf_safe_so_test.map "foo_prot" "foo_hidden"
arch_specific_safe_fold icf_safe_so_test_2.stdout icf_safe_so_test_1.stdout icf_safe_so_test.map "foo_prot" "foo_internal"
arch_specific_safe_fold icf_safe_so_test_2.stdout icf_safe_so_test_1.stdout icf_safe_so_test.map "foo_prot" "foo_static"
arch_specific_safe_fold icf_safe_so_test_2.stdout icf_safe_so_test_1.stdout icf_safe_so_test.map "foo_hidden" "foo_internal"
arch_specific_safe_fold icf_safe_so_test_2.stdout icf_safe_so_test_1.stdout icf_safe_so_test.map "foo_hidden" "foo_static"
arch_specific_safe_fold icf_safe_so_test_2.stdout icf_safe_so_test_1.stdout icf_safe_so_test.map "foo_internal" "foo_static"
check_nofold icf_safe_so_test_1.stdout "foo_glob" "bar_glob"
